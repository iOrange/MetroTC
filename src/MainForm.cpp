#include "MainForm.h"
#include "dds_utils.h"
#include <Commctrl.h>
#include "../resource.h"
#include "ChooseFolderDlg.h"
#include "Hyperlinks.h"

#include <lz4/lz4.h>
#include <crunch/inc/crn_decomp.h>


#define WM_MY_CONVERSION_FINISHED   (WM_USER + 1)


MainForm::MainForm()
    // form
    : mForm(nullptr)
    // radio buttons
    , mRadio2033(nullptr)
    , mRadioLL(nullptr)
    , mRadioExodus(nullptr)
    // buttons to choose source
    , mBtnChooseOneFile(nullptr)
    , mBtnChooseFolder(nullptr)
    // path and progress
    , mEditPath(nullptr)
    , mProgressBar(nullptr)
    // buttons to stop, convert and exit
    , mBtnStop(nullptr)
    , mBtnConvert(nullptr)
    , mBtnExit(nullptr)
    // about button
    , mBtnAbout(nullptr)
    // other
    , mMetroVersion(MetroVersion::_2033)
    , mConversionInProgress(false)
    , mStopRequested(true)
{
}
MainForm::~MainForm() {
}

MainForm* gThis = nullptr;
void MainForm::Run() {
    gThis = this;

    HINSTANCE myInstance = ::GetModuleHandle(nullptr);
    ::DialogBox(myInstance, MAKEINTRESOURCE(IDD_MAINFORM), nullptr, [](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)->INT_PTR {
        return gThis->DlgProc(hDlg, msg, wParam, lParam);
    });
}


int MainForm::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    int result = 0;

    switch (msg) {
        case WM_INITDIALOG: {
            mForm = hDlg;
            this->InitComponents();

            result = 1;
        } break;

        case WM_CLOSE: {
            this->OnExitButton();
        } break;

        case WM_COMMAND: {
            const int command = HIWORD(wParam);
            const int ctrlID = LOWORD(wParam);

            switch (ctrlID) {
                case IDC_EDIT_PATH: {
                    if (command == EN_CHANGE) {
                        this->OnEditPathTextChanged();
                    }
                } break;

                case IDC_EDIT_DEST_PATH: {
                    if (command == EN_CHANGE) {
                        this->OnTargetPathTextChanged();
                    }
                } break;

                case IDC_CHECK_DST_SAME_AS_SRC: {
                    this->OnCheckTargetSameAsSourceChanged();
                } break;

                case IDC_BUTTON_ONEFILE: {
                    this->OnChooseOneFileButton();
                } break;

                case IDC_BUTTON_FOLDER: {
                    this->OnChooseFolderButton();
                } break;

                case IDC_BUTTON_DST_FOLDER: {
                    this->OnChooseTargetFolderButton();
                } break;

                case IDC_BUTTON_CONVERT: {
                    this->OnConvertButton();
                } break;

                case IDC_BUTTON_STOP: {
                    this->OnStopButton();
                } break;

                case IDC_BUTTON_EXIT: {
                    this->OnExitButton();
                } break;

                case IDC_BUTTON_ABOUT: {
                    this->OnAboutButton();
                } break;

                case IDC_STATIC_GAMERU: {
                    ShellExecute(hDlg, TEXT("open"), TEXT("https://www.gameru.net"), nullptr, nullptr, SW_SHOWNORMAL);
                    return TRUE;
                } break;

            }
        } break;

        case WM_MY_CONVERSION_FINISHED: {
            if (mThread.joinable()) {
                mThread.join();
            }

            if (!lParam) {
                this->ShowMessage(L"Conversion completed");
            } else {
                //#NOTE_SK: if lParam is non-zero - we requested exit
                ::EndDialog(mForm, LOWORD(wParam));
            }
        } break;
    }

    return result;
}

void MainForm::OnEditPathTextChanged() {
    if (this->IsTargetPathSameAsSource()) {
        this->SetEditTargetPathText(this->GetEditPathText());
    }

    this->CheckPaths();
}

void MainForm::OnTargetPathTextChanged() {
    this->CheckPaths();
}

void MainForm::OnCheckTargetSameAsSourceChanged() {
    if (this->IsTargetPathSameAsSource()) {
        ::EnableWindow(mEditTargetPath, FALSE);
        ::EnableWindow(mBtnChooseTargetFolder, FALSE);
        this->SetEditTargetPathText(this->GetEditPathText());
    } else {
        ::EnableWindow(mEditTargetPath, TRUE);
        ::EnableWindow(mBtnChooseTargetFolder, TRUE);
        this->OnTargetPathTextChanged();
    }
}

void MainForm::OnChooseOneFileButton() {
    WCHAR szFile[2048] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = mForm;
    ofn.hInstance = ::GetModuleHandle(nullptr);
    ofn.lpstrFilter = TEXT("Metro textures\0*.512;*.1024;*.2048;*.512c;*.1024c;*.2048c\0\0");
    ofn.lpstrTitle = TEXT("Select Metro texture");
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 2047;

    if (::GetOpenFileName(&ofn)) {
        this->SetEditPathText(szFile);
    }
}

void MainForm::OnChooseFolderButton() {
    fs::path path = ChooseFolderDialog::ChooseFolder(L"Select folder with Metro textures", mForm);
    if (!path.empty()) {
        this->SetEditPathText(path.native());
    }
}

void MainForm::OnChooseTargetFolderButton() {
    fs::path path = ChooseFolderDialog::ChooseFolder(L"Select the destination folder", mForm);
    if (!path.empty()) {
        this->SetEditTargetPathText(path.native());
    }
}

void MainForm::OnConvertButton() {
    this->Convert();
}

void MainForm::OnStopButton() {
    mStopRequested = true;
}

void MainForm::OnExitButton() {
    this->OnStopButton();
    ::PostMessage(mForm, WM_MY_CONVERSION_FINISHED, 0, 1);
}

void MainForm::OnAboutButton() {
    this->ShowMessage(L"Created by Sergii 'iOrange' Kudlai\r\nExclusively for Gameru.net\r\n2019");
}

void MainForm::InitComponents() {
    HICON icon = reinterpret_cast<HICON>(::LoadImage(::GetModuleHandle(nullptr),
                                         MAKEINTRESOURCE(IDI_ICON1),
                                         IMAGE_ICON,
                                         ::GetSystemMetrics(SM_CXSMICON),
                                         ::GetSystemMetrics(SM_CYSMICON),
                                         0));
    if (icon) {
        ::PostMessage(mForm, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    }

    // radio buttons
    mRadio2033                  = ::GetDlgItem(mForm, IDC_RADIO_2033);
    mRadioLL                    = ::GetDlgItem(mForm, IDC_RADIO_LASTLIGHT);
    mRadioExodus                = ::GetDlgItem(mForm, IDC_RADIO_EXODUS);
    // buttons to choose source / dest
    mBtnChooseOneFile           = ::GetDlgItem(mForm, IDC_BUTTON_ONEFILE);
    mBtnChooseFolder            = ::GetDlgItem(mForm, IDC_BUTTON_FOLDER);
    mBtnChooseTargetFolder      = ::GetDlgItem(mForm, IDC_BUTTON_DST_FOLDER);
    // path and progress
    mEditPath                   = ::GetDlgItem(mForm, IDC_EDIT_PATH);
    mCheckWithSubfolders        = ::GetDlgItem(mForm, IDC_CHECK_WITH_SUBFOLDERS);
    mEditTargetPath             = ::GetDlgItem(mForm, IDC_EDIT_DEST_PATH);
    mCheckTargetSameAsSource    = ::GetDlgItem(mForm, IDC_CHECK_DST_SAME_AS_SRC);
    mProgressBar                = ::GetDlgItem(mForm, IDC_PROGRESS_BAR);
    // buttons to stop, convert and exit
    mBtnStop                    = ::GetDlgItem(mForm, IDC_BUTTON_STOP);
    mBtnConvert                 = ::GetDlgItem(mForm, IDC_BUTTON_CONVERT);
    mBtnExit                    = ::GetDlgItem(mForm, IDC_BUTTON_EXIT);
    // about button
    mBtnAbout                   = ::GetDlgItem(mForm, IDC_BUTTON_ABOUT);

    ::CheckDlgButton(mForm, IDC_CHECK_DST_SAME_AS_SRC, TRUE);
    this->OnCheckTargetSameAsSourceChanged();

    ::EnableWindow(mBtnStop, FALSE);
    ::EnableWindow(mBtnConvert, FALSE);

    ::CheckRadioButton(mForm, IDC_RADIO_2033, IDC_RADIO_EXODUS, IDC_RADIO_2033);


    ConvertStaticToHyperlink(mForm, IDC_STATIC_GAMERU);
}

const std::wstring& MainForm::GetFormTitle() const {
    static std::wstring str;

    WCHAR title[2048] = { 0 };
    ::GetWindowText(mForm, title, 2047);

    str.assign(title);

    return str;
}

const std::wstring& MainForm::GetEditPathText() const {
    static std::wstring str;

    WCHAR text[2048] = { 0 };
    ::GetWindowText(mEditPath, text, 2047);

    str.assign(text);

    return str;
}

void MainForm::SetEditPathText(const std::wstring& text) const {
    ::SetWindowText(mEditPath, text.c_str());
}

const std::wstring& MainForm::GetEditTargetPathText() const {
    static std::wstring str;

    WCHAR text[2048] = { 0 };
    ::GetWindowText(mEditTargetPath, text, 2047);

    str.assign(text);

    return str;
}

void MainForm::SetEditTargetPathText(const std::wstring& text) const {
    ::SetWindowText(mEditTargetPath, text.c_str());
}

bool MainForm::IsSubfoldersIncluded() const {
    return ::IsDlgButtonChecked(mForm, IDC_CHECK_WITH_SUBFOLDERS) == TRUE;
}

bool MainForm::IsTargetPathSameAsSource() const {
    return ::IsDlgButtonChecked(mForm, IDC_CHECK_DST_SAME_AS_SRC) == TRUE;
}

void MainForm::CheckPaths() {
    bool srcPathOk = false, dstPathOk = true;

    const std::wstring& srcPath = this->GetEditPathText();

    std::error_code err;
    fs::directory_entry srcEntry(srcPath, err);
    srcPathOk = srcEntry.exists();

    if (!this->IsTargetPathSameAsSource()) {
        const std::wstring& dstPath = this->GetEditTargetPathText();
        fs::directory_entry dstEntry(dstPath, err);
        dstPathOk = dstEntry.exists();
    }

    ::EnableWindow(mBtnConvert, (srcPathOk && dstPathOk) ? TRUE : FALSE);
}

void MainForm::ChangeUI(const bool conversionStarted) {
    ::EnableWindow(mEditPath, conversionStarted ? FALSE : TRUE);

    if (conversionStarted) {
        ::EnableWindow(mEditTargetPath, FALSE);
        ::EnableWindow(mCheckTargetSameAsSource, FALSE);
        ::EnableWindow(mBtnChooseTargetFolder, FALSE);
    } else {
        ::EnableWindow(mCheckTargetSameAsSource, TRUE);
        this->OnCheckTargetSameAsSourceChanged();
    }

    ::EnableWindow(mBtnStop, conversionStarted ? TRUE : FALSE);
    ::EnableWindow(mBtnConvert, conversionStarted ? FALSE : TRUE);

    ::EnableWindow(mRadio2033, conversionStarted ? FALSE : TRUE);
    ::EnableWindow(mRadioLL, conversionStarted ? FALSE : TRUE);
    ::EnableWindow(mRadioExodus, conversionStarted ? FALSE : TRUE);
}

void MainForm::ShowMessage(const std::wstring& message, const bool isError) {
    const std::wstring& title = this->GetFormTitle();

    ::MessageBox(mForm, message.c_str(), title.c_str(), MB_OK | (isError ? MB_ICONERROR : MB_ICONINFORMATION));
}

void MainForm::Convert() {
    if (mConversionInProgress) {
        return;
    }

    if (BST_CHECKED == ::IsDlgButtonChecked(mForm, IDC_RADIO_2033)) {
        mMetroVersion = MetroVersion::_2033;
    } else if (BST_CHECKED == ::IsDlgButtonChecked(mForm, IDC_RADIO_LASTLIGHT)) {
        mMetroVersion = MetroVersion::LastLight;
    } else {
        mMetroVersion = MetroVersion::Exodus;
    }

    const std::wstring& srcText = this->GetEditPathText();
    const std::wstring& dstText = this->GetEditTargetPathText();

    std::error_code err;
    fs::path srcPath(srcText), dstPath(dstText);
    fs::directory_entry srcEntry(srcPath, err), dstEntry(dstPath, err);
    if (srcEntry.exists()) {
        if (srcEntry.is_regular_file()) {
            if (!this->ConvertOneFile(srcPath)) {
                this->ShowMessage(L"Failed to convert:\r\n" + srcText, true);
            } else {
                this->ShowMessage(L"Successfully converted:\r\n" + srcText);
            }
        } else {
            mThread = std::thread([this, srcPath, dstPath]() {
                mStopRequested = false;
                mConversionInProgress = true;

                this->ChangeUI(true);
                this->ConvertFolder(srcPath, dstPath, this->IsSubfoldersIncluded());
                this->ChangeUI(false);

                mConversionInProgress = false;

                ::PostMessage(mForm, WM_MY_CONVERSION_FINISHED, 0, 0);
            });
        }
    }
}


enum class TextureFormat {
    BC1,
    BC3,
    BC7
};

struct TextureInfo {
    size_t  resolution;
    bool    exodus;
    bool    crunched;
};

static void DecrunchData(BytesBuffer& src) {
    crnd::crn_texture_info info;
    info.m_struct_size = sizeof(crnd::crn_texture_info);
    if (crnd::crnd_get_texture_info(src.data(), static_cast<uint32_t>(src.size()), &info)) {
        crnd::crnd_unpack_context ctx = crnd::crnd_unpack_begin(src.data(), static_cast<uint32_t>(src.size()));

        const size_t blockSize = (info.m_format == cCRNFmtDXT1) ? 8 : 16;

        const size_t ddsSize = (info.m_format == cCRNFmtDXT1) ?
                               DDS_GetCompressedSizeBC1(info.m_width, info.m_height, info.m_levels) :
                               DDS_GetCompressedSizeBC7(info.m_width, info.m_height, info.m_levels);
        BytesBuffer dst(ddsSize);

        uint8_t* dstPtr = dst.data();

        size_t size = info.m_width;
        for (size_t mip = 0; mip < info.m_levels; ++mip) {
            const size_t numBlocks = (size / 4);
            const size_t pitch = numBlocks * blockSize;
            const size_t mipSize = numBlocks * pitch;

            if (!crnd::crnd_unpack_level(ctx, (void**)&dstPtr, static_cast<uint32_t>(mipSize), static_cast<uint32_t>(pitch), static_cast<uint32_t>(mip))) {
                crnd::crnd_unpack_end(ctx);
                break;
            }

            dstPtr += mipSize;

            size >>= 1;
            if (size < 4) {
                size = 4;
            }
        }

        crnd::crnd_unpack_end(ctx);

        src.swap(dst);
    }
}

static void UncompressLZ4(BytesBuffer& src, const TextureInfo& info) {
    const size_t numMips = (info.resolution == 512) ? 10 : 1;
    const size_t ddsSize = DDS_GetCompressedSizeBC7(info.resolution, info.resolution, numMips);

    BytesBuffer dst(ddsSize);
    const int lz4Result = LZ4_decompress_safe(reinterpret_cast<const char*>(src.data()), reinterpret_cast<char*>(dst.data()),
                                              static_cast<int>(src.size()), static_cast<int>(dst.size()));

    if (lz4Result == static_cast<int>(ddsSize)) {
        src.swap(dst);
    }
}

static void Bc7ToBc3(BytesBuffer& src, const TextureInfo& info) {
    const size_t numMips = (info.resolution == 512) ? 10 : 1;

    size_t rgbaSize = 0;
    size_t size = info.resolution;
    for (size_t mip = 0; mip < numMips; ++mip) {
        rgbaSize += (size * size * 4);

        size >>= 1;
        if (size < 4) {
            size = 4;
        }
    }
    BytesBuffer rgba(rgbaSize);

    BytesBuffer dst(src.size());

    const uint8_t* srcPtr = src.data();
    uint8_t* dstPtr = dst.data();

    size = info.resolution;
    for (size_t mip = 0; mip < numMips; ++mip) {
        const size_t numBlocks = (size / 4);
        const size_t pitch = numBlocks * 16;
        const size_t mipSize = numBlocks * pitch;

        DDS_DecompressBC7(srcPtr, rgba.data(), size, size);
        DDS_CompressBC3(rgba.data(), dstPtr, size, size);

        dstPtr += mipSize;
        srcPtr += mipSize;

        size >>= 1;
        if (size < 4) {
            size = 4;
        }
    }

    src.swap(dst);
}

static BytesBuffer LoadTextureData(const fs::path& path, const TextureInfo& info) {
    BytesBuffer result;

    std::ifstream file(path, std::ifstream::binary);
    if (file.good()) {
        file.seekg(0, std::ios::end);
        result.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(result.data()), result.size());
        file.close();

        if (info.crunched) {
            DecrunchData(result);
        } else if (info.exodus) {
            UncompressLZ4(result, info);
            Bc7ToBc3(result, info);
        }
    }

    return std::move(result);
}

static size_t NumMipsFromResolution(const size_t resolution) {
    size_t result = 0;

    DWORD index;
    if (_BitScanReverse64(&index, resolution)) {
        result = static_cast<size_t>(index) + 1;
    }

    return result;
}

bool MainForm::ConvertOneFile(const fs::path& path, const fs::path& dstFolder) {
    bool result = false;

    const std::wstring& pathStr = path.native();

    fs::path outFolder = dstFolder.empty() ? path.parent_path() : dstFolder;

    const std::wstring textureBaseName = path.parent_path() / path.stem();

    const std::wstring outBaseName = outFolder / path.stem();
    fs::path outputPath(outBaseName + L".dds");

    const bool isCrunched = (pathStr.back() == L'c');

    std::error_code err;
    fs::directory_entry fileDDS(outputPath, err);
    if (fileDDS.exists()) {
        return true;
    }

    TextureInfo info;
    info.crunched = isCrunched;
    info.exodus = mMetroVersion == MetroVersion::Exodus;
    info.resolution = 512;

    fs::directory_entry file512(textureBaseName + (isCrunched ? L".512c" : L".512"), err);
    if (file512.exists()) { // O_o
        fs::directory_entry file1024(textureBaseName + (isCrunched ? L".1024c" : L".1024"), err);
        fs::directory_entry file2048(textureBaseName + (isCrunched ? L".2048c" : L".2048"), err);

        const size_t maxResolution = file2048.exists() ? 2048 : (file1024.exists() ? 1024 : 512);
        const size_t maxMips = NumMipsFromResolution(maxResolution);

        BytesBuffer data512 = LoadTextureData(file512.path(), info);
        const TextureFormat format = (mMetroVersion == MetroVersion::Exodus ? TextureFormat::BC7 :
                                     (data512.size() == 174776 ? TextureFormat::BC1 : TextureFormat::BC3));

        std::ofstream file(outputPath, std::ofstream::binary);
        if (file.good()) {
            file.write(reinterpret_cast<const char*>(&cDDSFileSignature), sizeof(cDDSFileSignature));

            DDSURFACEDESC2 ddsHdr;
            DDS_MakeDX9Header(ddsHdr, maxResolution, maxResolution, maxMips, format == TextureFormat::BC1);
            file.write(reinterpret_cast<const char*>(&ddsHdr), sizeof(ddsHdr));

            if (file2048.exists()) {
                info.resolution = 2048;
                BytesBuffer data2048 = LoadTextureData(file2048.path(), info);
                file.write(reinterpret_cast<const char*>(data2048.data()), data2048.size());
            }
            if (file1024.exists()) {
                info.resolution = 1024;
                BytesBuffer data1024 = LoadTextureData(file1024.path(), info);
                file.write(reinterpret_cast<const char*>(data1024.data()), data1024.size());
            }
            file.write(reinterpret_cast<const char*>(data512.data()), data512.size());

            file.flush();
            file.close();

            result = true;
        }
    }

    return result;
}

void MainForm::ConvertFolder(const fs::path& path, const fs::path& dstPath, const bool withSubfolders) {
    std::error_code err;

    const bool targetSameAsSource = fs::equivalent(path, dstPath);

    std::queue<fs::directory_entry> folders;
    folders.push(fs::directory_entry(path, err));

    // collect all textures
    std::vector<fs::directory_entry> files;
    while (!folders.empty()) {
        const fs::directory_entry& folder = folders.front();

        if (folder.is_directory()) {
            for (const fs::directory_entry& subEntry : fs::directory_iterator(folder.path())) {
                if (subEntry.is_regular_file()) {
                    const std::wstring extension = subEntry.path().extension();
                    if (extension == L".512" || extension == L".512c") {
                        files.push_back(subEntry);
                    }
                } else if (subEntry.is_directory() && withSubfolders) {
                    folders.push(subEntry);
                }
            }
        }

        folders.pop();
    }

    // convert all textures
    const size_t totalTextures = files.size();

    ::SendMessage(mProgressBar, PBM_SETRANGE32, 0, static_cast<LPARAM>(totalTextures));
    ::SendMessage(mProgressBar, PBM_SETSTEP, 1, 0);

    for (const fs::directory_entry& file : files) {
        fs::path fileSourcePath = file.path();

        fs::path fileDestFolder = fileSourcePath.parent_path();
        if (!targetSameAsSource) {
            fs::path relPath = fs::relative(fileDestFolder, path);
            fileDestFolder = fs::absolute(dstPath / relPath);

            fs::create_directories(fileDestFolder, err);
        }

        this->ConvertOneFile(file.path(), fileDestFolder);
        ::SendMessage(mProgressBar, PBM_STEPIT, 0, 0);

        if (mStopRequested) {
            break;
        }
    }
}
