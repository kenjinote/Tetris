// Tetris.cpp : アプリケーションのエントリ ポイントを定義します。
//

#pragma comment(lib, "d2d1")
#pragma comment(lib, "windowscodecs")

#include "framework.h"
#include "Tetris.h"

#define MAX_LOADSTRING 100

#define BLOCK_SIZE 32

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int, LPVOID);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
class Field;

template<class Interface> inline void SafeRelease(Interface** ppInterfaceToRelease)
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

HRESULT LoadResourceBitmap(
    ID2D1RenderTarget* pRenderTarget,
    IWICImagingFactory* pIWICFactory,
    PCWSTR resourceName,
    PCWSTR resourceType,
    UINT destinationWidth,
    UINT destinationHeight,
    ID2D1Bitmap** ppBitmap
)
{
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICStream* pStream = NULL;
    IWICFormatConverter* pConverter = NULL;
    IWICBitmapScaler* pScaler = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void* pImageFile = NULL;
    DWORD imageFileSize = 0;

    // Locate the resource.
    imageResHandle = FindResourceW(GetModuleHandle(0), resourceName, resourceType);
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        // Load the resource.
        imageResDataHandle = LoadResource(GetModuleHandle(0), imageResHandle);

        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        // Lock it to get a system memory pointer.
        pImageFile = LockResource(imageResDataHandle);

        hr = pImageFile ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        // Calculate the size.
        imageFileSize = SizeofResource(GetModuleHandle(0), imageResHandle);

        hr = imageFileSize ? S_OK : E_FAIL;

    }
    if (SUCCEEDED(hr))
    {
        // Create a WIC stream to map onto the memory.
        hr = pIWICFactory->CreateStream(&pStream);
    }
    if (SUCCEEDED(hr))
    {
        // Initialize the stream with the memory pointer and size.
        hr = pStream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(pImageFile),
            imageFileSize
        );
    }
    if (SUCCEEDED(hr))
    {
        // Create a decoder for the stream.
        hr = pIWICFactory->CreateDecoderFromStream(
            pStream,
            NULL,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
        );
    }
    if (SUCCEEDED(hr))
    {
        // Create the initial frame.
        hr = pDecoder->GetFrame(0, &pSource);
    }
    if (SUCCEEDED(hr))
    {
        // Convert the image format to 32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->Initialize(
            pSource,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut
        );
    }
    if (SUCCEEDED(hr))
    {
        //create a Direct2D bitmap from the WIC bitmap.
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pConverter,
            NULL,
            ppBitmap
        );

    }

    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);

    return hr;
}


// ゲーム変数
class Block {
public:
    char x;
    char y;
    Block(char cx, char cy) {
        x = cx;
        y = cy;
    }
    void translate(char cx, char cy) {
        x += cx;
        y += cy;
    }
    void rotate() {
        char x_ = x;
        x = -y;
        y = x_;
    }
    void draw(ID2D1HwndRenderTarget* pRenderTarget, ID2D1Brush* pBrush, ID2D1Bitmap* pBitmap) {
        D2D1_RECT_F rectangle1 = D2D1::RectF((FLOAT)(x * BLOCK_SIZE + 0.3), (FLOAT)(y * BLOCK_SIZE + 0.3), (FLOAT)((x + 1) * BLOCK_SIZE - 0.3), (FLOAT)((y + 1) * BLOCK_SIZE - 0.3));
        if (pBitmap) {
            pRenderTarget->DrawBitmap(
                pBitmap,
                rectangle1,
                1.0,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }
        else
        {
            pRenderTarget->FillRectangle(&rectangle1, pBrush);
        }
    }
};


class Mino {
public:
    char x;
    char y;
    char rot;
    char shape;
    Mino(char cx, char cy, char crot, char cshape) {
        x = cx;
        y = cy;
        rot = crot;
        shape = cshape;
    }
    Block** calcBlocks() {
        Block** blocks = new Block * [4];
        switch (shape) {
        case 0: //T
            blocks[0] = new Block(-1, 0);
            blocks[1] = new Block(0, 0);
            blocks[2] = new Block(0, -1);
            blocks[3] = new Block(1, 0);
            break;
        case 1: //Z
            blocks[0] = new Block(-1, -1);
            blocks[1] = new Block(0, -1);
            blocks[2] = new Block(0, 0);
            blocks[3] = new Block(1, 0);
            break;
        case 2: //S
            blocks[0] = new Block(-1, 0);
            blocks[1] = new Block(0, 0);
            blocks[2] = new Block(0, -1);
            blocks[3] = new Block(1, -1);
            break;
        case 3: //L
            blocks[0] = new Block(-1, -2);
            blocks[1] = new Block(-1, -1);
            blocks[2] = new Block(-1, 0);
            blocks[3] = new Block(0, 0);
            break;
        case 4: //J
            blocks[0] = new Block(0, -2);
            blocks[1] = new Block(0, -1);
            blocks[2] = new Block(-1, 0);
            blocks[3] = new Block(0, 0);
            break;
        case 5: //O
            blocks[0] = new Block(-1, -1);
            blocks[1] = new Block(-1, 0);
            blocks[2] = new Block(0, 0);
            blocks[3] = new Block(0, -1);
            break;
        case 6: //I
            blocks[0] = new Block(-2, 0);
            blocks[1] = new Block(-1, 0);
            blocks[2] = new Block(0, 0);
            blocks[3] = new Block(1, 0);
            break;
        }
        //rotの値だけ、90度右回転する。回数は剰余で0～3に丸めこむ。
        //rotがマイナスの場合剰余がバグるので、4の倍数を足して調整
        rot = (40000000 + rot) % 4;
        for (char r = 0; r < rot; r++) {
            for (char i = 0; i < 4; i++) {
                blocks[i]->rotate();
            }
        }
        //ブロックのグローバル座標（Field上の座標）に変換する
        for (char i = 0; i < 4; i++) {
            blocks[i]->translate(x, y);
        }
        return blocks;
    }
    void draw(ID2D1HwndRenderTarget* pRenderTarget, ID2D1Brush* pBrush, ID2D1Bitmap* pBitmap) {
        Block** blocks = calcBlocks();
        for (char i = 0; i < 4; i++) {
            blocks[i]->draw(pRenderTarget, pBrush, pBitmap);
        }
        for (char i = 0; i < 4; i++) {
            delete blocks[i];
        }
        delete[]blocks;
    }
    Mino* copy() {
        return new Mino(x, y, rot, shape);
    }
};


class Field {
    bool tiles[21][12];
public:
    Field() : tiles{
        // ※2次元配列について:
        // https://developer.mozilla.org/ja/docs/Web/JavaScript/Reference/Global_Objects/Array
        // 「2次元配列を生成する」を見てください。
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    }{}
    bool tileAt(char x, char y) {
        if (x < 0 || x >= 12 || y < 0 || y >= 21) return 1; //画面外
        return tiles[y][x];
    }
    void putBlock(char x, char y) {
        tiles[y][x] = 1;
    }
    void putBlock(Block* block) {
        tiles[block->y][block->x] = 1;
    }
    bool isfillline(char y) {
        for (char i = 0; i < 12; i++) {
            if (tiles[y][i] == 0) return false;
        }
        return true;
    }
    char findLineFilled() {
        for (char y = 0; y < 20; y++) {
            // ※every関数について: https://developer.mozilla.org/ja/docs/Web/JavaScript/Reference/Global_Objects/Array/every
            bool isFilled = isfillline(y);
            if (isFilled) return y;
        }
        return -1;
    }
    void cutLine(char y) {
        char initLine[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        for (; y > 0; y--)
        {
            memcpy(tiles[y], tiles[y - 1], sizeof(tiles[y - 1]));
        }
    }
    void draw(ID2D1HwndRenderTarget* pRenderTarget, ID2D1Brush* pBrush, ID2D1Bitmap* pBitmap) {
        for (char y = 0; y < 21; y++) {
            for (char x = 0; x < 12; x++) {
                if (tileAt(x, y) == 0) continue;
                Block(x, y).draw(pRenderTarget, pBrush, pBitmap);
            }
        }
    }
};


class Game {
    Mino* mino;
    char minoVx;
    char minoDrop; // 0:なし 1:↓キーによるスピードアップ 2:↑キーによる強制落下
    char minoVr;
    Field* field;
    char fc = 0;
    ID2D1Factory* m_pD2DFactory;
    IWICImagingFactory* m_pWICFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pBlackBrush;
    ID2D1Bitmap* m_pBitmap;
public:
    Game() {
        mino = makeMino();
        minoVx = 0;
        minoDrop = 0;
        minoVr = 0;
        field = new Field();
        fc = 0;
        m_pD2DFactory = nullptr;
        m_pWICFactory = nullptr;
        m_pRenderTarget = nullptr;
        m_pBlackBrush = nullptr;
        m_pBitmap = nullptr;
    }
    ~Game()
    {
        delete mino;
        delete field;
        SafeRelease(&m_pD2DFactory);
        SafeRelease(&m_pWICFactory);
        SafeRelease(&m_pRenderTarget);
        SafeRelease(&m_pBlackBrush);
        SafeRelease(&m_pBitmap);
        CoUninitialize();
    }
    void init() {
        if (FAILED(CoInitialize(0)))
        {
            MessageBox(GetDesktopWindow(), TEXT("COM の初期化に失敗しました。"), 0, 0);
            ExitProcess(0);
            return;
        }
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory)))
        {
            MessageBox(GetDesktopWindow(), TEXT("Direct2D の初期化に失敗しました。"), 0, 0);
            ExitProcess(0);
            return;
        }
        if (FAILED(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IWICImagingFactory,
                reinterpret_cast<void**>(&m_pWICFactory)
            ) ) )
        {
            MessageBox(GetDesktopWindow(), TEXT("WIC コンポーネントの初期化に失敗しました。"), 0, 0);
            ExitProcess(0);
            return;
        }
    }
    static Mino* makeMino() {
        return new Mino(5, 2, 0, rand() % 7);
    }
    static bool isMinoMovable(Mino* mino, Field* field) {
        bool ret = true;
        Block** blocks = mino->calcBlocks();
        for (char i = 0; i < 4; i++) {
            if (field->tileAt(blocks[i]->x, blocks[i]->y))
            {
                ret = false;
                break;
            }
        }
        for (char i = 0; i < 4; i++) {
            delete blocks[i];
        }
        delete[]blocks;
        return ret;
    }
    void fix() {
        Block** blocks = mino->calcBlocks();
        for (char i = 0; i < 4; i++) {
            field->putBlock(blocks[i]);
        }
        for (char i = 0; i < 4; i++) {
            delete blocks[i];
        }
        delete[]blocks;
    }
    void proc(HWND hWnd) {
        // 落下
        if (minoDrop || fc % 30 == 0) {
            Mino * futureMino = mino->copy();
            if (minoDrop == 2)
            {
                do {
                    futureMino->y += 1;
                } while (isMinoMovable(futureMino, field));
                mino->y = futureMino->y - 1;
                // 接地
                fix();
                // 次のミノ作成
                delete mino;
                mino = makeMino();
            }
            else
            {
                futureMino->y += 1;
                if (isMinoMovable(futureMino, field)) {
                    mino->y += 1;
                }
                else {
                    // 接地
                    fix();
                    // 次のミノ作成
                    delete mino;
                    mino = makeMino();
                }
            }
            delete futureMino;
            // 消去
            char line;
            while ((line = field->findLineFilled()) != -1) {
                field->cutLine(line);
            }
            minoDrop = 0;
        }
        // 左右移動
        if (minoVx != 0) {
            Mino* futureMino = mino->copy();
            futureMino->x += minoVx;
            if (isMinoMovable(futureMino, field)) {
                mino->x += minoVx;
            }
            delete futureMino;
            minoVx = 0;
        }
        // 回転
        if (minoVr != 0) {
            Mino* futureMino = mino->copy();
            futureMino->rot += minoVr;
            if (isMinoMovable(futureMino, field)) {
                mino->rot += minoVr;
            }
            delete futureMino;
            minoVr = 0;
        }

        // 描画
        draw(hWnd);

        fc++;
    }
    void key(unsigned int vkey) {
        switch (vkey) {
        case VK_LEFT:
            minoVx = -1;
            break;
        case VK_RIGHT:
            minoVx = 1;
            break;
        case VK_UP:
            minoDrop = 2;
            break;
        case VK_DOWN:
            minoDrop = 1;
            break;
        case 'Z':
            minoVr = -1;
            break;
        case 'X':
            minoVr = 1;
            break;
        }
    }
    void draw(HWND hWnd) {
        HRESULT hr = S_OK;
        if (!m_pRenderTarget)
        {
            RECT rect;
            GetClientRect(hWnd, &rect);
            D2D1_SIZE_U size = D2D1::SizeU(rect.right, rect.bottom);
            hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &m_pRenderTarget);
            if (SUCCEEDED(hr))
                hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pBlackBrush);
            if (SUCCEEDED(hr))
                hr = LoadResourceBitmap(m_pRenderTarget, m_pWICFactory, MAKEINTRESOURCE(IDB_PNG1), L"PNG", 16, 16, &m_pBitmap);
        }
        if (SUCCEEDED(hr))
        {
            D2D1_SIZE_F renderTargetSize = m_pRenderTarget->GetSize();
            m_pRenderTarget->BeginDraw();
            m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
            mino->draw(m_pRenderTarget, m_pBlackBrush, m_pBitmap);
            field->draw(m_pRenderTarget, m_pBlackBrush, m_pBitmap);
            hr = m_pRenderTarget->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET)
            {
                hr = S_OK;
                SafeRelease(&m_pRenderTarget);
                SafeRelease(&m_pBlackBrush);
                SafeRelease(&m_pBitmap);
            }
        }
    }
    void resize(UINT32 width, UINT32 height) {
        if (m_pRenderTarget)
        {
            D2D1_SIZE_U size = { width, height };
            m_pRenderTarget->Resize(size);
        }
    }
};


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TETRIS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    Game* game = new Game();
    game->init();

    // アプリケーション初期化の実行:
    HWND hWnd = InitInstance(hInstance, nCmdShow, game);
    if (!hWnd)
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TETRIS));

    MSG msg = {};
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {

            game->proc(hWnd);
            //ゲームの処理
            //17ミリ秒スレッド待機
        }
    }
    delete game;

    return (int) msg.wParam;
}


//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TETRIS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TETRIS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow, LPVOID game)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, game);

   if (!hWnd)
   {
      return 0;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return hWnd;
}


//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Game* game;
    switch (message)
    {
    case WM_CREATE:
        game = (Game*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_KEYDOWN:
        game->key(wParam);
        break;
    case WM_SIZE:
        game->resize(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_DISPLAYCHANGE:
        InvalidateRect(hWnd, 0, 0);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
