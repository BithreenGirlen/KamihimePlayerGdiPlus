

#include "kamihime_scene_player.h"

#include <gdiplus.h>
#include <math.h>

#pragma comment (lib,"Gdiplus.lib")


CKamihimeScenePlayer::CKamihimeScenePlayer(HWND hWnd)
	:m_hRetWnd(hWnd)
{
	Gdiplus::GdiplusStartupInput GdiPlusStartupInput;
	Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_GdiPlusToken, &GdiPlusStartupInput, NULL);
}

CKamihimeScenePlayer::~CKamihimeScenePlayer()
{
	EndThreadpoolTimer();

	if (m_GdiPlusToken != 0)
	{
		Gdiplus::GdiplusShutdown(m_GdiPlusToken);
	}
}
/*�t�H���_�ݒ�*/
bool CKamihimeScenePlayer::SetFolder(const wchar_t* pzFolderPath)
{
	if (pzFolderPath == nullptr || m_hRetWnd == nullptr)return false;
	m_wstrFolder = std::wstring(pzFolderPath).append(L"\\/");

	bool bRet = FindImages();
	ResetScale();

	return bRet;
}
/*�`��*/
bool CKamihimeScenePlayer::DisplayImage()
{
	if (m_image_info.empty() || m_nIndex >= m_image_info.size() || m_hRetWnd == nullptr)
	{
		return false;
	}

	Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromHWND(m_hRetWnd, FALSE);
	if (pGraphics == nullptr)return false;

	pGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
	pGraphics->SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
	pGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighSpeed);
	pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
	pGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);

	Gdiplus::REAL fScale = static_cast<Gdiplus::REAL>(::round(m_dbScale * 1000) / 1000);
	pGraphics->ScaleTransform(fScale, fScale);
	
	ImageInfo s = m_image_info.at(m_nIndex);

	UINT uiDiv = s.uiWidth / Portion::kWidth;
	if (uiDiv > 1)
	{
		StartThreadpoolTimer();

		UINT uiWidth = s.uiWidth / uiDiv;
		UINT uiHeight = s.uiHeight;
		INT iStride = s.iStride / uiDiv;

		BYTE* pixels = static_cast<BYTE*>(malloc(static_cast<size_t>(iStride * uiHeight)));
		if (pixels != nullptr)
		{
			for (UINT i = 0; i < uiHeight; ++i)
			{
				memcpy(pixels + iStride * i, s.pixels.data() + iStride * m_uiPortion + s.iStride * i, iStride);
			}
			Gdiplus::Bitmap bitmap(uiWidth, uiHeight, iStride, PixelFormat32bppRGB, pixels);
			Gdiplus::Rect Rect{0, 0, static_cast<INT>(uiWidth), static_cast<INT>(uiHeight)};
			pGraphics->DrawImage(&bitmap, Rect, m_iXOffset, m_iYOffset, uiWidth, uiHeight, Gdiplus::UnitPixel);

			free(pixels);
		}

		++m_uiPortion;
		if (m_uiPortion >= uiDiv)m_uiPortion = 0;
	}
	else
	{
		Gdiplus::Bitmap bitmap(s.uiWidth, s.uiHeight, s.iStride, PixelFormat32bppRGB, s.pixels.data());
		Gdiplus::Rect Rect{0, 0, static_cast<INT>(s.uiWidth), static_cast<INT>(s.uiHeight)};
		pGraphics->DrawImage(&bitmap, Rect, m_iXOffset, m_iYOffset, s.uiWidth, s.uiHeight, Gdiplus::UnitPixel);
	}

	delete pGraphics;

	return true;
}
/*���摜*/
void CKamihimeScenePlayer::Next()
{
	EndThreadpoolTimer();
	++m_nIndex;
	if (m_nIndex >= m_image_info.size())m_nIndex = 0;

	Update();
}
/*�g��*/
void CKamihimeScenePlayer::UpScale()
{
	if (m_dbScale < 4.0)
	{
		m_dbScale += 0.05;
		ResizeWindow();
	}
}
/*�k��*/
void CKamihimeScenePlayer::DownScale()
{
	if (m_dbScale > 0.5)
	{
		m_dbScale -= 0.05;
		ResizeWindow();
	}
}
/*�����\��*/
void CKamihimeScenePlayer::ResetScale()
{
	m_dbScale = 1.0;
	m_iXOffset = 0;
	m_iYOffset = 0;
	m_interval = Portion::kInterval;
	ResizeWindow();
}
/*���_�ʒu�ړ�*/
void CKamihimeScenePlayer::SetOffset(int iX, int iY)
{
	AdjustOffset(iX, iY);
	Update();
}
/*�R�}�������*/
void CKamihimeScenePlayer::SpeedUp()
{
	if (m_interval > 1)
	{
		--m_interval;
	}
}
/*�R�}���茸��*/
void CKamihimeScenePlayer::SpeedDown()
{
	if (m_interval < 1000)
	{
		++m_interval;
	}
}
/*����*/
void CKamihimeScenePlayer::Clear()
{
	m_image_info.clear();
	m_nIndex = 0;
}
/*�摜�t�@�C���T��*/
bool CKamihimeScenePlayer::FindImages()
{
	Clear();

	WIN32_FIND_DATAW find_file_data;
	std::wstring wstrFile = m_wstrFolder + L"*.jpg";
	HANDLE hFind = ::FindFirstFileW(wstrFile.c_str(), &find_file_data);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::wstring wstr = m_wstrFolder + find_file_data.cFileName;
				LoadImageToMemory(wstr.c_str());
			}
		} while (::FindNextFileW(hFind, &find_file_data));
		::FindClose(hFind);
	}

	return m_image_info.size() > 0;
}
/*�摜�t�@�C����荞��*/
bool CKamihimeScenePlayer::LoadImageToMemory(const wchar_t* pzFilePath)
{
	ImageInfo s;

	Gdiplus::Bitmap bitmap(pzFilePath, PixelFormat32bppPARGB);
	Gdiplus::Status status = bitmap.RotateFlip(Gdiplus::RotateFlipType::Rotate270FlipNone);
	if (status != Gdiplus::Status::Ok)return false;

	s.uiWidth = bitmap.GetWidth();
	s.uiHeight = bitmap.GetHeight();

	Gdiplus::Rect Rect{0, 0, static_cast<INT>(s.uiWidth), static_cast<INT>(s.uiHeight)};
	Gdiplus::BitmapData bmp_data{};
	status = bitmap.LockBits(&Rect, Gdiplus::ImageLockModeRead, PixelFormat32bppRGB, &bmp_data);
	if (status != Gdiplus::Status::Ok)return false;

	s.iStride = bmp_data.Stride;
	s.pixels.resize(static_cast<size_t>(s.iStride * s.uiHeight));

	for (UINT i = 0; i < s.uiHeight; ++i)
	{
		memcpy(s.pixels.data() + s.iStride * i, static_cast<BYTE*>(bmp_data.Scan0) + bmp_data.Stride * i, s.iStride);
	}

	status = bitmap.UnlockBits(&bmp_data);

	m_image_info.push_back(s);

	return true;
}
/*��ʍX�V*/
void CKamihimeScenePlayer::Update()
{
	if (m_hRetWnd != nullptr)
	{
		::InvalidateRect(m_hRetWnd, NULL, TRUE);
	}
}
/*���g���@����*/
void CKamihimeScenePlayer::ResizeWindow()
{
	if (!m_image_info.empty() && m_hRetWnd != nullptr)
	{
		RECT rect;
		::GetWindowRect(m_hRetWnd, &rect);
		int iX = static_cast<int>(::round(Portion::kWidth * (m_dbScale * 1000) / 1000));
		int iY = static_cast<int>(::round(m_image_info.at(0).uiHeight * (m_dbScale * 1000) / 1000));
		rect.right = iX + rect.left;
		rect.bottom = iY + rect.top;
		::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
		::SetWindowPos(m_hRetWnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);

		AdjustOffset(0, 0);
		Update();
	}

}
/*���_�ʒu����*/
void CKamihimeScenePlayer::AdjustOffset(int iXAddOffset, int iYAddOffset)
{
	if (!m_image_info.empty() && m_hRetWnd != nullptr)
	{
		int iX = static_cast<int>(::round(Portion::kWidth * (m_dbScale * 1000) / 1000));
		int iY = static_cast<int>(::round(m_image_info.at(0).uiHeight * (m_dbScale * 1000) / 1000));

		RECT rc;
		::GetClientRect(m_hRetWnd, &rc);

		int iClientWidth = rc.right - rc.left;
		int iClientHeight = rc.bottom - rc.top;

		int iXOffsetMax = iX > iClientWidth ? static_cast<int>(::floor((iX - iClientWidth) / ((m_dbScale * 1000) / 1000))) : 0;
		int iYOffsetMax = iY > iClientHeight ? static_cast<int>(::floor((iY - iClientHeight) / ((m_dbScale * 1000) / 1000))) : 0;

		m_iXOffset += iXAddOffset;
		if (m_iXOffset < 0) m_iXOffset = 0;
		if (m_iXOffset > iXOffsetMax)m_iXOffset = iXOffsetMax;
		m_iYOffset += iYAddOffset;
		if (m_iYOffset < 0) m_iYOffset = 0;
		if (m_iYOffset > iYOffsetMax)m_iYOffset = iYOffsetMax;
	}

}
/*�R�}����J�n*/
void CKamihimeScenePlayer::StartThreadpoolTimer()
{
	if (m_timer != nullptr)return;

	m_timer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);
	if (m_timer != nullptr)
	{
		m_uiPortion = 0;

		FILETIME FileDueTime{};
		ULARGE_INTEGER ulDueTime{};
		ulDueTime.QuadPart = static_cast<ULONGLONG>(-(1LL * 10 * 1000 * m_interval));
		FileDueTime.dwHighDateTime = ulDueTime.HighPart;
		FileDueTime.dwLowDateTime = ulDueTime.LowPart;
		::SetThreadpoolTimer(m_timer, &FileDueTime, 0, 0);
	}

}
/*�R�}����I��*/
void CKamihimeScenePlayer::EndThreadpoolTimer()
{
	if (m_timer != nullptr)
	{
		::SetThreadpoolTimer(m_timer, nullptr, 0, 0);
		::WaitForThreadpoolTimerCallbacks(m_timer, TRUE);
		::CloseThreadpoolTimer(m_timer);
		m_timer = nullptr;
	}
}
/*�R�}���菈���X���b�h�v�[��*/
void CKamihimeScenePlayer::TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer)
{
	CKamihimeScenePlayer* pThis = static_cast<CKamihimeScenePlayer*>(Context);
	if (pThis != nullptr)
	{
		pThis->Update();

		FILETIME FileDueTime{};
		ULARGE_INTEGER ulDueTime{};
		ulDueTime.QuadPart = static_cast<ULONGLONG>(-(1LL * 10 * 1000 * pThis->m_interval));
		FileDueTime.dwHighDateTime = ulDueTime.HighPart;
		FileDueTime.dwLowDateTime = ulDueTime.LowPart;

		::SetThreadpoolTimer(Timer, &FileDueTime, 0, 0);
	}

}

