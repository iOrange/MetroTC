#include "mycommon.h"

#include <Windows.h>


class MainForm {
    enum class MetroVersion {
        _2033,
        LastLight,
        Exodus
    };

public:
    MainForm();
    ~MainForm();

    void            Run();

private:
    // UI
    int                 DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    void                InitComponents();
    
    const std::wstring& GetFormTitle() const;
    const std::wstring& GetEditPathText() const;
    void                SetEditPathText(const std::wstring& text) const;
    const std::wstring& GetEditTargetPathText() const;
    void                SetEditTargetPathText(const std::wstring& text) const;

    bool                IsTargetPathSameAsSource() const;

    void                OnEditPathTextChanged();
    void                OnTargetPathTextChanged();
    void                OnCheckTargetSameAsSourceChanged();
    void                OnChooseOneFileButton();
    void                OnChooseFolderButton();
    void                OnChooseTargetFolderButton();
    void                OnConvertButton();
    void                OnStopButton();
    void                OnExitButton();
    void                OnAboutButton();

    void                ChangeUI(const bool conversionStarted);
    void                ShowMessage(const std::wstring& message, const bool isError = false);

    // conversion
    void                Convert();
    bool                ConvertOneFile(const fs::path& path);
    void                ConvertFolder(const fs::path& path);

private:
    // form
    HWND            mForm;
    // radio buttons
    HWND            mRadio2033;
    HWND            mRadioLL;
    HWND            mRadioExodus;
    // buttons to choose source / dest
    HWND            mBtnChooseOneFile;
    HWND            mBtnChooseFolder;
    HWND            mBtnChooseTargetFolder;
    // path and progress
    HWND            mEditPath;
    HWND            mCheckWithSubfolders;
    HWND            mEditTargetPath;
    HWND            mCheckTargetSameAsSource;
    HWND            mProgressBar;
    // buttons to stop, convert and exit
    HWND            mBtnStop;
    HWND            mBtnConvert;
    HWND            mBtnExit;
    // about button
    HWND            mBtnAbout;

    // other
    MetroVersion    mMetroVersion;
    std::thread     mThread;
    volatile bool   mConversionInProgress;
    volatile bool   mStopRequested;
};
