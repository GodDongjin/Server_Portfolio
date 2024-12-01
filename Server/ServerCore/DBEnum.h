#pragma once
#include "pch.h"

enum class CREATE_PLAYER_ERROR
{
	CREATE_PLAYER_NONE = 0,        // �÷��̾ ���� �������� ����
	CREATE_PLAYER_FAIL = 1,        // �÷��̾� ���� ����
	CREATE_PLAYER_SUCCESS = 2,     // �÷��̾� ���� ����
	CREATE_PLAYER_DUPLICATION = 3, // �÷��̾� �̸� �ߺ�
};

enum class SELECT_PLAYER_ERROR
{
	SELECT_PLAYER_NONE = 0,       // �÷��̾� ���� ������ ����
	SELECT_PLAYER_FAIL = 1,       // �÷��̾� ���� ����
	SELECT_PLAYER_SUCCESS = 2,    // �÷��̾� ���� ����
};