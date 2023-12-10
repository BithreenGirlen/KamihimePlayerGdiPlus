# KamihimePlayerGdiPlus
2023-08-29

 This is GdiPlus version of [KamihimePlayer](https://github.com/BithreenGirlen/KamihimePlayer).
 
This was deprecated because of its slow rendering though, has an advantage over Direct2D version; a bit better scaling interpolation.  
This is left just for GpiPlus sample programme, so lacking some of the utilities implemented in Direct2D version of KamihimePlayer and never to be updated.

There are two points totally different from  Direct2D version; one impoting images and another drawing a part of the image.  
## Image imporing to memory

``` cpp
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
```

## Drawing a part of image
``` cpp

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
```
