
#include <shobjidl.h>
#include <atlbase.h>
#include <mfapi.h>

#include "kamihime_media_player.h"

#pragma comment (lib,"Mfplat.lib")

/*�Đ��@�N���X�|�C���^�i�[*/
void CKamihimePlayerNotify::SetPlayer(void* arg)
{
	m_pPlayer = arg;
}
/*�Đ��C�x���g����*/
void CKamihimePlayerNotify::OnMediaEngineEvent(DWORD Event, DWORD_PTR param1, DWORD param2)
{
	CKamihimeMediaPlayer* pPlayer = static_cast<CKamihimeMediaPlayer*>(m_pPlayer);
	if (pPlayer != nullptr)
	{
		switch (Event)
		{
		case MF_MEDIA_ENGINE_EVENT_ENDED:
			pPlayer->AutoNext();
			break;
		}
	}
}

CKamihimeMediaPlayer::CKamihimeMediaPlayer()
{
	Initialise();
}

CKamihimeMediaPlayer::~CKamihimeMediaPlayer()
{
	Release();
}
/*�t�H���_�ݒ�*/
bool CKamihimeMediaPlayer::SetFolder(const wchar_t* pzFolderPath)
{
	if (pzFolderPath == nullptr)return false;
	m_wstrFolder = std::wstring(pzFolderPath).append(L"\\/");

	FindAudios();

	return Play();
}
/*�Đ�*/
bool CKamihimeMediaPlayer::Play()
{
	if (m_pmfEngine != nullptr && !m_audio_files.empty() && m_nIndex <= m_audio_files.size())
	{
		m_pmfEngine->SetSource(const_cast<BSTR>(m_audio_files.at(m_nIndex).c_str()));
		return SUCCEEDED(m_pmfEngine->Play());
	}
	return false;
}
/*����*/
void CKamihimeMediaPlayer::Next()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_nIndex;
	if (m_nIndex >= m_audio_files.size())m_nIndex = 0;
	Play();
}
/*�߂�*/
void CKamihimeMediaPlayer::Back()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	--m_nIndex;
	if (m_nIndex >= m_audio_files.size())m_nIndex = m_audio_files.size() - 1;
	Play();
}
/*�Đ����[�v�ݒ�*/
BOOL CKamihimeMediaPlayer::SwitchLoop()
{
	if (m_pmfEngine != nullptr)
	{
		m_iLoop ^= TRUE;
		if (FAILED(m_pmfEngine->SetLoop(m_iLoop)))
		{
			m_iLoop ^= TRUE;
		}
	}
	return m_iLoop;
}
/*�����ݒ�*/
BOOL CKamihimeMediaPlayer::SwitchMute()
{
	if (m_pmfEngine != nullptr)
	{
		m_iMute ^= TRUE;
		if (FAILED(m_pmfEngine->SetMuted(m_iMute)))
		{
			m_iMute ^= TRUE;
		}
	}
	return m_iMute;
}
/*���ʎ擾*/
double CKamihimeMediaPlayer::GetCurrentVolume()
{
	if (m_pmfEngine != nullptr)
	{
		return m_pmfEngine->GetVolume();
	}
	return 100.0;
}
/*�Đ����x�擾*/
double CKamihimeMediaPlayer::GetCurrentRate()
{
	if (m_pmfEngine != nullptr)
	{
		return m_pmfEngine->GetPlaybackRate();
	}
	return 1.0;
}
/*���ʐݒ�*/
bool CKamihimeMediaPlayer::SetCurrentVolume(double dbVolume)
{
	if (m_pmfEngine != nullptr)
	{
		return SUCCEEDED(m_pmfEngine->SetVolume(dbVolume));
	}
	return false;
}
/*�Đ����x�ݒ�*/
bool CKamihimeMediaPlayer::SetCurrentRate(double dbRate)
{
	if (m_pmfEngine != nullptr)
	{
		if (dbRate != m_pmfEngine->GetDefaultPlaybackRate())
		{
			m_pmfEngine->SetPlaybackRate(dbRate);
		}
		return SUCCEEDED(m_pmfEngine->SetDefaultPlaybackRate(dbRate));
	}
	return false;
}
/*��������*/
void CKamihimeMediaPlayer::AutoNext()
{
	if (m_nIndex < m_audio_files.size()-1)Next();
}
/*������*/
bool CKamihimeMediaPlayer::Initialise()
{
	m_hrComInit = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(m_hrComInit))return false;

	m_hrMfStart = ::MFStartup(MF_VERSION);
	if (FAILED(m_hrMfStart))return false;

	CComPtr<IMFMediaEngineClassFactory> pmfFactory;

	HRESULT hr = ::CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pmfFactory));
	if (FAILED(hr))return false;

	CComPtr<IMFAttributes> pmfAttributes;

	hr = ::MFCreateAttributes(&pmfAttributes, 1);
	if (FAILED(hr))return false;

	m_pmfNotify = new CKamihimePlayerNotify();
	hr = pmfAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, reinterpret_cast<IUnknown*>(m_pmfNotify));
	if (FAILED(hr))
	{
		m_pmfNotify->Release();
		return false;
	}

	m_pmfNotify->SetPlayer(this);

	hr = pmfFactory->CreateInstance(MF_MEDIA_ENGINE_WAITFORSTABLE_STATE, pmfAttributes, &m_pmfEngine);
	if (FAILED(hr))
	{
		m_pmfNotify->Release();
		return false;
	}

	m_pmfEngine->SetVolume(0.5);

	return true;
}
/*�j��*/
void CKamihimeMediaPlayer::Release()
{
	if (m_pmfNotify != nullptr)
	{
		m_pmfNotify->Release();
	}

	if (m_pmfEngine != nullptr)
	{
		m_pmfEngine->Release();
	}

	if (SUCCEEDED(m_hrMfStart))
	{
		::MFShutdown();
	}

	if (SUCCEEDED(m_hrComInit))
	{
		::CoUninitialize();
	}
}
/*����*/
void CKamihimeMediaPlayer::Clear()
{
	m_audio_files.clear();
	m_nIndex = 0;
}
/*�����t�@�C���T��*/
bool CKamihimeMediaPlayer::FindAudios()
{
	Clear();

	WIN32_FIND_DATAW find_file_data;
	std::wstring wstrFile = m_wstrFolder + L"*.mp3";
	HANDLE hFind = ::FindFirstFileW(wstrFile.c_str(), &find_file_data);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::wstring wstr = m_wstrFolder + find_file_data.cFileName;
				m_audio_files.push_back(wstr);
			}
		} while (::FindNextFileW(hFind, &find_file_data));
		::FindClose(hFind);
	}

	return m_audio_files.size() > 0;
}
