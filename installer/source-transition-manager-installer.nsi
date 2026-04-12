; Source Transition Manager for OBS
; NSIS Installer Script

!define PRODUCT_NAME "Source Transition Manager for OBS"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "LetsDoVideo"
!define PRODUCT_WEB_SITE "https://letsdovideo.com"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\SourceTransitionManager"

; Modern UI
!include "MUI2.nsh"
!include "LogicLib.nsh"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "SourceTransitionManager-Setup.exe"
InstallDir "$PROGRAMFILES64\obs-studio"
InstallDirRegKey HKLM "SOFTWARE\OBS Studio" ""
RequestExecutionLevel admin
SetCompressor /SOLID lzma

; UI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_WELCOMEPAGE_TITLE "Welcome to Source Transition Manager Setup"
!define MUI_WELCOMEPAGE_TEXT "This will install Source Transition Manager for OBS.$\r$\n$\r$\nSource Transition Manager gives you a convenient dock to quickly view and adjust show/hide transitions for your sources.$\r$\n$\r$\nClick Next to continue."
!define MUI_FINISHPAGE_TITLE "Source Transition Manager Installation Complete"
!define MUI_FINISHPAGE_TEXT "Source Transition Manager has been installed successfully.$\r$\n$\r$\nStart OBS and look for Source Transition Manager in the Docks menu to get started."

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${ROOT_DIR}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
; VC++ 2015-2022 REDISTRIBUTABLE CHECK
; ---------------------------------------------------------------------------
Section "-VCRedist" SecVCRedist
    ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
    ${If} $0 == 1
        Goto VCRedistDone
    ${EndIf}

    SetOutPath "$TEMP"
    File "${ROOT_DIR}\installer\VC_redist.x64.exe"
    ExecWait '"$TEMP\VC_redist.x64.exe" /install /quiet /norestart'
    Delete "$TEMP\VC_redist.x64.exe"

    VCRedistDone:
SectionEnd

; ---------------------------------------------------------------------------
; INSTALLER SECTIONS
; ---------------------------------------------------------------------------
Section "Source Transition Manager Plugin" SecMain
    SectionIn RO

    SetOverwrite on

    ; Plugin DLL -> obs-plugins/64bit/
    SetOutPath "$INSTDIR\obs-plugins\64bit"
    File "${ROOT_DIR}\dist\obs-plugins\64bit\source-transition-manager.dll"

    ; Locale -> data/obs-plugins/source-transition-manager/locale/
    SetOutPath "$INSTDIR\data\obs-plugins\source-transition-manager\locale"
    File "${ROOT_DIR}\dist\data\obs-plugins\source-transition-manager\locale\en-US.ini"

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\SourceTransitionManager-Uninstall.exe"

    ; Write registry for Add/Remove Programs
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\SourceTransitionManager-Uninstall.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1

SectionEnd

; ---------------------------------------------------------------------------
; UNINSTALLER
; ---------------------------------------------------------------------------
Section "Uninstall"

    Delete "$INSTDIR\obs-plugins\64bit\source-transition-manager.dll"
    Delete "$INSTDIR\data\obs-plugins\source-transition-manager\locale\en-US.ini"
    RMDir "$INSTDIR\data\obs-plugins\source-transition-manager\locale"
    RMDir "$INSTDIR\data\obs-plugins\source-transition-manager"
    Delete "$INSTDIR\SourceTransitionManager-Uninstall.exe"

    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"

    MessageBox MB_OK "Source Transition Manager has been uninstalled successfully."

SectionEnd

; ---------------------------------------------------------------------------
; DIRECTORY PAGE CUSTOMIZATION
; ---------------------------------------------------------------------------
Function .onVerifyInstDir
    IfFileExists "$INSTDIR\obs64.exe" DirOK
    IfFileExists "$INSTDIR\bin\64bit\obs64.exe" DirOK
        MessageBox MB_YESNO \
            "The selected folder does not appear to be an OBS installation.$\r$\n$\r$\nInstall here anyway?" \
            IDYES DirOK
        Abort
    DirOK:
FunctionEnd
