#pragma once

class GraphicScene
{
protected:
	CComPtr<ID2D1Factory> factory;
	CComPtr<ID2D1HwndRenderTarget> target;
	float p2dX, p2dY;

	virtual HRESULT create_device_independent_resource() = 0;
	virtual HRESULT release_device_independent_resource() = 0;
	virtual HRESULT create_device_dependent_resource() = 0;
	virtual HRESULT release_device_dependent_resource() = 0;
	virtual void calculate_layout() = 0;
	virtual void render_scene() = 0;

	HRESULT create_graphic_resource(HWND wnd);

	float pixel2dipX(float val) const
	{
		return val * p2dX;
	}

	float pixel2dipY(float val) const
	{
		return val * p2dY;
	}

public:
	GraphicScene() : p2dX(1.F), p2dY(1.F) {}
	virtual ~GraphicScene() {}

	HRESULT create();
	HRESULT render(HWND wnd);
	HRESULT resize(int width, int height);
	HRESULT release();
};


inline HRESULT GraphicScene::create_graphic_resource(HWND wnd)
{
	HRESULT res = S_OK;
	if (!!target)
		return res;

	RECT rc;
	GetClientRect(wnd, &rc);
	D2D1_SIZE_U sz = D2D1::SizeU(rc.right, rc.bottom);

	res = factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(wnd, sz),
		&target
	);

	if (SUCCEEDED(res))
		res = create_device_dependent_resource();
	if (SUCCEEDED(res))
		calculate_layout();

	return res;
}


inline HRESULT GraphicScene::create()
{
	HRESULT res = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
	if (SUCCEEDED(res))
		create_device_independent_resource();
	return res;
}


inline HRESULT GraphicScene::render(HWND wnd)
{
	HRESULT res = create_graphic_resource(wnd);
	if (FAILED(res))
		return res;

	assert(target.p);
	target->BeginDraw();
	render_scene();
	res = target->EndDraw();

	if (res == D2DERR_RECREATE_TARGET)
	{
		release_device_dependent_resource();
		target.Release();
	}
	return res;
}


inline HRESULT GraphicScene::resize(int width, int height)
{
	HRESULT res = S_OK;
	if (!target)
		return res;

	res = target->Resize(D2D1::SizeU(width, height));
	if (SUCCEEDED(res))
		calculate_layout();
	return res;
}


inline HRESULT GraphicScene::release()
{
	release_device_dependent_resource();
	release_device_independent_resource();
	return S_OK;
}



