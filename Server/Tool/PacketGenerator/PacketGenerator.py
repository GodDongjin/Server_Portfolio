import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

try:
	import jinja2
except ModuleNotFoundError:
	raise SystemExit('Install dependencies: python -m pip install -r Server/Tool/PacketGenerator/requirements.txt')

sys.dont_write_bytecode = True
import ProtoParser


TOOL_DIR = Path(__file__).resolve().parent
ROOT_DIR = TOOL_DIR.parents[2]


def generate_packets(target, staging_dir):
	if target == 'server':
		common_dir = ROOT_DIR / 'Server/Common/protoc-21.12-win64/bin'
		project_dir = ROOT_DIR / 'Server/GameServer'
		protobuf_dir = project_dir / 'Protobuf'
		protocol_name = 'Protocol.proto'
		output = 'ServerPacketHandler'
		recv_prefix, send_prefix = 'REQ_', 'ACK_'
	else:
		common_dir = ROOT_DIR / 'TestClient/Common/protoc-28.1-win64/bin'
		project_dir = ROOT_DIR / 'TestClient/TestClient'
		protobuf_dir = project_dir / 'Protocol'
		protocol_name = 'protocol.proto'
		output = 'ClientPacketHandler'
		recv_prefix, send_prefix = 'ACK_', 'REQ_'

	# 파일 이름 대소문자까지 서버와 맞춰 두어 생성되는 include 이름을 유지한다.
	for name in ('Enum.proto', 'Struct.proto', 'Protocol.proto'):
		source = common_dir / (protocol_name if name == 'Protocol.proto' else name)
		(staging_dir / name).write_bytes(source.read_bytes())

	# 모든 생성이 성공하기 전까지 실제 프로젝트 파일은 변경하지 않는다.
	subprocess.run([
		str(common_dir / 'protoc.exe'), '--proto_path=.', '--cpp_out=.',
		'Enum.proto', 'Struct.proto', 'Protocol.proto'
	], cwd=staging_dir, check=True)

	parser = ProtoParser.ProtoParser(1000, recv_prefix, send_prefix)
	parser.parse_proto(staging_dir / 'Protocol.proto')
	if not parser.total_pkt:
		raise ValueError('No protocol packets found: ' + target)

	env = jinja2.Environment(
		loader=jinja2.FileSystemLoader(TOOL_DIR / 'Templates'),
		undefined=jinja2.StrictUndefined,
		keep_trailing_newline=True, newline_sequence='\r\n')
	text = env.get_template(output + '.h').render(parser=parser, output=output)

	files = {}
	for stem in ('Enum', 'Struct', 'Protocol'):
		for extension in ('.pb.h', '.pb.cc'):
			name = stem + extension
			files[protobuf_dir / name] = (staging_dir / name).read_bytes()
	# 한글 주석을 Visual Studio에서도 동일하게 읽도록 BOM을 포함한다.
	files[project_dir / 'Packet' / (output + '.h')] = text.encode('utf-8-sig')
	return files


def main():
	arg_parser = argparse.ArgumentParser(description='Generate server/client protocol files')
	arg_parser.add_argument('--target', choices=('server', 'client', 'all'), default='all')
	arg_parser.add_argument('--check', action='store_true', help='check generated files without changing them')
	args = arg_parser.parse_args()
	targets = ('server', 'client') if args.target == 'all' else (args.target,)

	try:
		files = {}
		with tempfile.TemporaryDirectory(prefix='packet-generator-') as directory:
			for target in targets:
				staging_dir = Path(directory) / target
				staging_dir.mkdir()
				files.update(generate_packets(target, staging_dir))

		# Git의 줄바꿈 변환은 코드 변경으로 취급하지 않는다.
		changed = [path for path, data in files.items()
			if not path.is_file() or path.read_bytes().replace(b'\r\n', b'\n') != data.replace(b'\r\n', b'\n')]
		if args.check:
			for path in changed:
				print('OUTDATED: ' + path.relative_to(ROOT_DIR).as_posix())
			if changed:
				return 1
			print('PASS: generated protocol files are up to date')
			return 0

		for path in changed:
			path.write_bytes(files[path])
			print('UPDATED: ' + path.relative_to(ROOT_DIR).as_posix())
		print('Generated ' + ', '.join(targets) + ' packets (' + str(len(changed)) + ' files changed)')
		return 0
	except (OSError, ValueError, subprocess.CalledProcessError, jinja2.TemplateError) as error:
		print('Packet generation failed: ' + str(error), file=sys.stderr)
		return 1


if __name__ == '__main__':
	sys.exit(main())
