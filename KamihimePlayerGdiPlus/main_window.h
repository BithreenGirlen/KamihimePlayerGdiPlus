#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <Windows.h>

#include <string>

#include "kamihime_scene_player.h"
#include "kamihime_media_player.h"

class CMainWindow
{
public:
	CMainWindow();
	~CMainWindow();
	bool Create(HINSTANCE hInstance);
	int MessageLoop();
	HWND GetHwnd()const { return m_hWnd;}
private:
	std::wstring m_class_name = L"Kamihime player window";
	std::wstring m_window_name = L"Kamihime player";
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(HWND hWnd);
	LRESULT OnDestroy();
	LRESULT OnClose();
	LRESULT OnPaint();
	LRESULT OnSize();
	LRESULT OnCommand(WPARAM wParam);
	LRESULT OnMouseWheel(WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnMButtonUp(WPARAM wParam, LPARAM lParam);

	enum Menu{kOpen = 1, kNextImage, kNextAudio, kBack, kPlay, kLoop, kVolume};
	enum MenuBar{kFolder, kImage, kAudio};
	POINT m_CursorPos{};
	bool m_bSpeedSet = false;

	void InitialiseMenuBar();

	void MenuOnOpen();
	void MenuOnNextImage();
	void MenuOnNextAudio();
	void MenuOnBack();
	void MenuOnPlay();
	void MenuOnLoop();
	void MenuOnVolume();

	CKamihimeScenePlayer* m_pKamihimeScenePlayer = nullptr;
	CKamihimeMediaPlayer* m_pKamihimeMediaPlayer = nullptr;
};

#endif //MAIN_WINDOW_H_