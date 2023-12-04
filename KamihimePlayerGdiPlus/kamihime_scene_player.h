#ifndef KAMIHIME_SCENE_PLAYER_H_
#define KAMIHIME_SCENE_PLAYER_H_

#include <Windows.h>

#include <string>
#include <vector>

struct ImageInfo
{
    UINT uiWidth;
    UINT uiHeight;
    INT iStride;
    std::vector<BYTE> pixels;
};

class CKamihimeScenePlayer
{
public:
    CKamihimeScenePlayer(HWND hWnd);
    ~CKamihimeScenePlayer();

    bool SetFolder(const wchar_t* pzFolderPath);
    bool DisplayImage();
    void Next();
    void UpScale();
    void DownScale();
    void ResetScale();
    void SetOffset(int iX, int iY);
    void SpeedUp();
    void SpeedDown();
private:
    ULONG_PTR m_GdiPlusToken = 0;

    enum Portion{kInterval = 16, kWidth = 900};

    std::wstring m_wstrFolder;
    HWND m_hRetWnd = nullptr;

    std::vector<ImageInfo> m_image_info;
    UINT m_uiPortion = 0;
    size_t m_nIndex = 0;

    double m_dbScale = 1.0;
    int m_iXOffset = 0;
    int m_iYOffset = 0;
    long long m_interval = Portion::kInterval;

    void Clear();
    bool FindImages();
    bool LoadImageToMemory(const wchar_t* pzFilePath);
    void Update();
    void ResizeWindow();
    void AdjustOffset(int iXAddOffset, int iYAddOffset);

    void StartThreadpoolTimer();
    void EndThreadpoolTimer();

    PTP_TIMER m_timer = nullptr;
    static void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);
};

#endif //KAMIHIME_SCENE_PLAYER_H_
