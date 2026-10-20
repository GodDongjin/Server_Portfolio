#include "../TestClient/Session/TestSession.h"
#include "../TestClient/NetWork/ClientServer.h"
#include "../TestClient/Packet/ClientPacketHandler.h"
#include "../TestClient/Utils/StringUtil.h"
#include "../TestClient/Utils/GlobalData.h"
#include <chrono>
#include <thread>

int WSAAPI test_wsa_send(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD,
	LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int WSAAPI test_wsa_recv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD,
	LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
BOOL WINAPI test_get_completion(HANDLE, LPDWORD, PULONG_PTR, LPOVERLAPPED*, DWORD);

// 제품 코드의 처리 경로를 그대로 사용하고, 이 실행 파일에서만 OS 호출 결과를 주입한다.
#define WSASend test_wsa_send
#define WSARecv test_wsa_recv
#include "../TestClient/Session/TestSession.cpp"
#undef WSASend
#undef WSARecv
#define GetQueuedCompletionStatus test_get_completion
#include "../TestClient/NetWork/ClientServer.cpp"
#undef GetQueuedCompletionStatus

namespace
{
	vector<string> send_requests;
	IocpEvent* last_send_event = nullptr;
	int32 send_error = WSA_IO_PENDING;
	int32 recv_error = WSA_IO_PENDING;
	atomic<bool> real_io = false;
	atomic<bool> inject_failed_completion = false;
	atomic<int32> failed_completions = 0;
	TestSession* completion_session = nullptr;

	void require_send(bool ok, const char* message)
	{
		if (!ok)
		{
			cerr << "FAIL: " << message << endl;
			abort();
		}
	}

	string join_buffers(const vector<WSABUF>& buffers)
	{
		string result;
		for (const auto& buffer : buffers) {
			result.append(buffer.buf, buffer.len);
		}
		return result;
	}

	shared_ptr<SendBuffer> make_buffer(const string& data)
	{
		auto buffer = make_shared<SendBuffer>(static_cast<int32>(data.size()));
		memcpy(buffer->get_buffer(), data.data(), data.size());
		buffer->close(static_cast<uint32>(data.size()));
		return buffer;
	}

	void reset_send()
	{
		send_requests.clear();
		last_send_event = nullptr;
		send_error = WSA_IO_PENDING;
		recv_error = WSA_IO_PENDING;
		real_io.store(false);
		inject_failed_completion.store(false);
		failed_completions.store(0);
		completion_session = nullptr;
	}

	void complete_send(const shared_ptr<TestSession>& session, uint32 bytes)
	{
		require_send(last_send_event != nullptr, "send operation is registered");
		session->dispatch(last_send_event, bytes);
	}

	template<typename Condition>
	void wait_until(Condition condition, const char* message)
	{
		const uint64 deadline = ::GetTickCount64() + 5000;
		while (!condition())
		{
			require_send(::GetTickCount64() < deadline, message);
			std::this_thread::sleep_for(chrono::milliseconds(1));
		}
	}

	struct ConnectedSession
	{
		ConnectedSession()
		{
			SOCKET listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			require_send(listener != INVALID_SOCKET, "create loopback listener");
			SOCKADDR_IN address{};
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			require_send(::bind(listener, reinterpret_cast<SOCKADDR*>(&address), sizeof(address)) == 0,
				"bind temporary loopback port");
			require_send(::listen(listener, 1) == 0, "listen");
			int32 length = sizeof(address);
			require_send(::getsockname(listener, reinterpret_cast<SOCKADDR*>(&address), &length) == 0,
				"get temporary port");
			session = make_shared<TestSession>();
			require_send(session->connect(address), "connect TestSession");
			peer = ::accept(listener, nullptr, nullptr);
			::closesocket(listener);
			require_send(peer != INVALID_SOCKET, "accept peer");
			DWORD timeout = 5000;
			require_send(::setsockopt(peer, SOL_SOCKET, SO_RCVTIMEO,
				reinterpret_cast<char*>(&timeout), sizeof(timeout)) == 0, "bound receiver wait");
		}

		~ConnectedSession()
		{
			session->disconnect();
			::closesocket(peer);
		}

		shared_ptr<TestSession> session;
		SOCKET peer = INVALID_SOCKET;
	};

	void test_send_event()
	{
		SendEvent event;
		vector<WSABUF> buffers;
		require_send(!event.consume_sent_bytes(1), "empty event rejects completion");
		const string expected = "abc12345XYZ";
		auto first = make_buffer("abc");
		auto second = make_buffer("12345");
		auto third = make_buffer("XYZ");
		event.send_Buffers_push(first);
		event.send_Buffers_push(second);
		event.send_Buffers_push(third);
		require_send(!event.consume_sent_bytes(0) && !event.consume_sent_bytes(12)
			&& !event.consume_sent_bytes(UINT32_MAX), "invalid sizes do not advance the cursor");
		size_t consumed = 0;
		for (uint32 bytes : { 1U, 4U, 3U, 2U, 1U })
		{
			event.build_wsabufs(buffers);
			require_send(join_buffers(buffers) == expected.substr(consumed), "unsent suffix is exact");
			require_send(event.consume_sent_bytes(bytes), "valid completion is consumed");
			consumed += bytes;
		}
		require_send(!event.has_pending_data(), "event completes at final byte");
		require_send(first->get_write_size() == 3 && second->get_write_size() == 5,
			"shared buffer contents and lengths are preserved");
		event.clear();
		event.send_Buffers_push(third);
		event.build_wsabufs(buffers);
		require_send(join_buffers(buffers) == "XYZ", "next batch starts from offset zero");
		cout << "PASS: send cursor, cross-buffer completions, invalid sizes and reset" << endl;
	}

	void test_partial_session_send()
	{
		reset_send();
		ConnectedSession fixture;
		auto session = fixture.session;
		auto first = make_buffer("abcdefgh");
		auto second = make_buffer("12345");
		auto third = make_buffer("XYZ");
		auto fourth = make_buffer("!?");
		weak_ptr<SendBuffer> weak_first = first, weak_second = second, weak_third = third, weak_fourth = fourth;
		session->send(first);
		session->send(second);
		session->send(third);
		first.reset();
		second.reset();
		third.reset();
		require_send(send_requests == vector<string>({ "abcdefgh" }), "only one send is outstanding");
		complete_send(session, 3);
		require_send(send_requests.back() == "defgh" && !weak_first.expired(), "partial tail is retained");
		session->send(fourth);
		fourth.reset();
		complete_send(session, 5);
		require_send(weak_first.expired() && send_requests.back() == "12345XYZ!?",
			"next queued batch starts after current batch finishes");
		complete_send(session, 6);
		require_send(send_requests.back() == "YZ!?", "completion crosses first queued buffer boundary");
		complete_send(session, 2);
		require_send(send_requests.back() == "!?", "boundary completion keeps only unsent suffix");
		complete_send(session, 2);
		require_send(weak_second.expired() && weak_third.expired() && weak_fourth.expired(),
			"buffers are released after whole batch completes");
		session->send(make_buffer("next"));
		require_send(send_requests.size() == 6 && send_requests.back() == "next",
			"new send starts after queue becomes idle");
		complete_send(session, 4);
		require_send(session->is_connected(), "partial completions do not disconnect");
		cout << "PASS: TestSession resumes partial sends before later queued packets" << endl;
	}

	void test_send_failures()
	{
		for (uint32 completion : { 0U, 9U })
		{
			reset_send();
			ConnectedSession fixture;
			auto buffer = make_buffer("12345678");
			weak_ptr<SendBuffer> weak_buffer = buffer;
			fixture.session->send(buffer);
			buffer.reset();
			complete_send(fixture.session, completion);
			require_send(!fixture.session->is_connected() && weak_buffer.expired(),
				"zero or oversized completion disconnects and releases buffers");
		}

		{
			reset_send();
			send_error = WSAENOBUFS;
			ConnectedSession fixture;
			fixture.session->send(make_buffer("failed"));
			require_send(!fixture.session->is_connected(), "immediate send failure disconnects");
			fixture.session->send(make_buffer("ignored"));
			require_send(send_requests.size() == 1, "disconnected session does not start another send");
		}

		{
			reset_send();
			ConnectedSession fixture;
			auto first = make_buffer("12345678");
			auto next = make_buffer("queued");
			weak_ptr<SendBuffer> weak_first = first, weak_next = next;
			fixture.session->send(first);
			fixture.session->send(next);
			first.reset();
			next.reset();
			send_error = WSAECONNRESET;
			complete_send(fixture.session, 3);
			require_send(!fixture.session->is_connected() && weak_first.expired() && weak_next.expired(),
				"failure while resending clears unsent and queued buffers");
		}

		{
			reset_send();
			ConnectedSession fixture;
			auto pending = make_buffer("pending");
			auto queued = make_buffer("queued");
			weak_ptr<SendBuffer> weak_pending = pending, weak_queued = queued;
			fixture.session->send(pending);
			fixture.session->send(queued);
			pending.reset();
			queued.reset();
			recv_error = WSAENOBUFS;
			require_send(!fixture.session->start() && !fixture.session->is_connected(),
				"receive registration failure disconnects");
			require_send(!weak_pending.expired() && weak_queued.expired(),
				"disconnect keeps only the outstanding send buffer until completion");
			complete_send(fixture.session, 0);
			require_send(weak_pending.expired(), "cancelled send completion releases pending buffer");
		}

		{
			reset_send();
			send_error = 0;
			ConnectedSession fixture;
			auto buffer = make_buffer("sync");
			weak_ptr<SendBuffer> weak_buffer = buffer;
			fixture.session->send(buffer);
			buffer.reset();
			require_send(!weak_buffer.expired(), "synchronous return still waits for IOCP completion");
			complete_send(fixture.session, 4);
			require_send(weak_buffer.expired(), "synchronous completion releases buffer once");
		}
		cout << "PASS: send failures, receive failure, cancellation and synchronous completion" << endl;
	}

	void test_failed_iocp_completion()
	{
		reset_send();
		ConnectedSession fixture;
		ClientServer server;
		require_send(server.CreateIocpHandle() && server.register_socket(fixture.session), "register IOCP");
		auto buffer = make_buffer("async failure");
		weak_ptr<SendBuffer> weak_buffer = buffer;
		fixture.session->send(buffer);
		buffer.reset();
		completion_session = fixture.session.get();
		inject_failed_completion.store(true);
		server.start_worker_thread(1);
		wait_until([&]() { return !fixture.session->is_connected() && weak_buffer.expired(); },
			"failed IOCP completion must release buffers and disconnect");
		server.stop();
		require_send(failed_completions.load() == 1 && send_requests.size() == 1,
			"failed completion cannot be mistaken for a successful partial send");
		cout << "PASS: ClientServer dispatches failed IOCP send completion for cleanup" << endl;
	}

	void test_real_stream()
	{
		reset_send();
		real_io.store(true);
		ConnectedSession fixture;
		ClientServer server;
		require_send(server.CreateIocpHandle() && server.register_socket(fixture.session), "register real IOCP");
		server.start_worker_thread(2);
		vector<string> chunks;
		string expected;
		for (int32 i = 0; i < 512; ++i)
		{
			chunks.emplace_back(4096, static_cast<char>('A' + i % 26));
			chunks.back().replace(0, to_string(i).size(), to_string(i));
			expected += chunks.back();
		}
		string received;
		thread receiver([&]()
		{
			char buffer[8192];
			while (received.size() < expected.size())
			{
				int32 count = ::recv(fixture.peer, buffer, sizeof(buffer), 0);
				require_send(count > 0, "receive complete stream before timeout");
				received.append(buffer, count);
			}
		});
		weak_ptr<SendBuffer> last_buffer;
		for (const string& chunk : chunks)
		{
			auto buffer = make_buffer(chunk);
			last_buffer = buffer;
			fixture.session->send(buffer);
		}
		receiver.join();
		require_send(received == expected, "real TCP stream preserves all bytes and queue order");
		wait_until([&]() { return last_buffer.expired(); }, "real completion releases final buffer");
		fixture.session->disconnect();
		server.stop();
		real_io.store(false);
		cout << "PASS: real Winsock + 2 IOCP workers transfer 2 MiB without loss or reordering" << endl;
	}
}

int WSAAPI test_wsa_send(SOCKET socket, LPWSABUF buffers, DWORD count, LPDWORD bytes,
	DWORD flags, LPWSAOVERLAPPED overlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE callback)
{
	if (real_io.load()) {
		return ::WSASend(socket, buffers, count, bytes, flags, overlapped, callback);
	}
	require_send(bytes == nullptr, "overlapped byte count comes from completion");
	send_requests.push_back(join_buffers(vector<WSABUF>(buffers, buffers + count)));
	last_send_event = static_cast<IocpEvent*>(overlapped);
	if (send_error == 0) {
		return 0;
	}
	::WSASetLastError(send_error);
	return SOCKET_ERROR;
}

int WSAAPI test_wsa_recv(SOCKET socket, LPWSABUF buffers, DWORD count, LPDWORD bytes,
	LPDWORD flags, LPWSAOVERLAPPED overlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE callback)
{
	if (real_io.load()) {
		return ::WSARecv(socket, buffers, count, bytes, flags, overlapped, callback);
	}
	::WSASetLastError(recv_error);
	return SOCKET_ERROR;
}

BOOL WINAPI test_get_completion(HANDLE handle, LPDWORD bytes, PULONG_PTR key,
	LPOVERLAPPED* overlapped, DWORD timeout)
{
	if (inject_failed_completion.exchange(false))
	{
		// 실패 완료의 바이트 수를 믿지 않고 0으로 전달하는지도 검증한다.
		*bytes = 3;
		*key = reinterpret_cast<ULONG_PTR>(completion_session);
		*overlapped = last_send_event;
		failed_completions++;
		::SetLastError(ERROR_OPERATION_ABORTED);
		return FALSE;
	}
	return ::GetQueuedCompletionStatus(handle, bytes, key, overlapped, timeout);
}


static void test_chat_target_count()
{
	ClientPacketHandler::Init();
	auto session = make_shared<TestSession>();
	GServerStats.expected_chat_recv.store(0);

	// 전송 요청만으로 추정치를 늘리지 않고 서버의 성공 응답을 기다린다.
	session->send_chat(L"room", Protocol::CHAT_STATE::CHAT_NORMAL, 1);
	session->send_chat(L"all", Protocol::CHAT_STATE::CHAT_ALL, 1);
	session->send_whisper_chat(L"target", L"whisper", Protocol::CHAT_STATE::CHAT_WHISPER, 1);
	require_send(GServerStats.expected_chat_recv.load() == 0, "send does not guess recipient count");

	auto acknowledge = [&](Protocol::CHAT_ERROR result, uint32 count)
	{
		Protocol::ACK_CHAT ack;
		ack.set_result(result);
		ack.set_target_count(count);
		const int32 payload_size = static_cast<int32>(ack.ByteSizeLong());
		vector<BYTE> data(sizeof(PacketHeader) + payload_size);
		PacketHeader header{ static_cast<uint16>(data.size()), PKT_ACK_CHAT };
		memcpy(data.data(), &header, sizeof(header));
		require_send(ack.SerializeToArray(data.data() + sizeof(header), payload_size), "serialize chat ACK");
		require_send(ClientPacketHandler::HandlePacket(session, data.data(), static_cast<int32>(data.size())),
			"chat ACK decoded through actual packet handler");
	};
	acknowledge(Protocol::CHAT_ERROR_SUCCESS, 3);
	acknowledge(Protocol::CHAT_ERROR_SUCCESS, 1);
	acknowledge(Protocol::CHAT_ERROR_SUCCESS, 0);
	require_send(GServerStats.expected_chat_recv.load() == 4, "server target counts are accumulated");
	acknowledge(Protocol::CHAT_ERROR_FAIL, 99);
	acknowledge(Protocol::CHAT_ERROR_NOT_TARGET, 99);
	require_send(GServerStats.expected_chat_recv.load() == 4, "failed requests never increase expected receives");
	cout << "PASS: chat statistics use successful ACK targets, including zero targets and failures" << endl;
}

int main()
{
	WSADATA data{};
	require_send(::WSAStartup(MAKEWORD(2, 2), &data) == 0, "Winsock startup");
	test_chat_target_count();
	test_send_event();
	test_partial_session_send();
	test_send_failures();
	test_failed_iocp_completion();
	test_real_stream();
	::WSACleanup();
	cout << "PASS: all client send tests" << endl;
}
