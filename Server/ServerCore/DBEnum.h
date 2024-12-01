#pragma once
#include "pch.h"

enum class CREATE_PLAYER_ERROR
{
	CREATE_PLAYER_NONE = 0,        // 플레이어가 아직 생성되지 않음
	CREATE_PLAYER_FAIL = 1,        // 플레이어 생성 실패
	CREATE_PLAYER_SUCCESS = 2,     // 플레이어 생성 성공
	CREATE_PLAYER_DUPLICATION = 3, // 플레이어 이름 중복
};

enum class SELECT_PLAYER_ERROR
{
	SELECT_PLAYER_NONE = 0,       // 플레이어 선택 오류가 없음
	SELECT_PLAYER_FAIL = 1,       // 플레이어 선택 실패
	SELECT_PLAYER_SUCCESS = 2,    // 플레이어 선택 성공
};