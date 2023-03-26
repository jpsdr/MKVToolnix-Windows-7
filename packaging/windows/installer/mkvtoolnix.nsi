!define PRODUCT_NAME "MKVToolNix"
!define PRODUCT_VERSION "75.0.0"
!define PRODUCT_VERSION_BUILD ""
!define PRODUCT_PUBLISHER "Moritz Bunkus"
!define PRODUCT_WEB_SITE "https://www.bunkus.org/videotools/mkvtoolnix/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\mkvtoolnix-gui.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

!define MTX_REGKEY "Software\bunkus.org"

#SetCompress off
SetCompressor /SOLID lzma
SetCompressorDictSize 64

!include "MUI2.nsh"
!include "file_association.nsh"
!include "nsDialogs.nsh"

# MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "mkvtoolnix-gui.ico"

# Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "HKCU"
!define MUI_LANGDLL_REGISTRY_KEY "${MTX_REGKEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

# Settings for the start menu group page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${PRODUCT_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "header_image.bmp"

# Settings for the finish page
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_TITLE_3LINES

# Welcome page
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome_finish_page.bmp"
!define MUI_WELCOMEPAGE_TITLE_3LINES

# Variables for custom dialogs
Var Dialog
Var Label1
Var Link1
Var Link2
Var Link3
Var CheckBox1
Var CheckBox1_State

!insertmacro MUI_PAGE_WELCOME
Page custom mediaFoundationFrameworkCheck mediaFoundationFrameworkCheckLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!insertmacro MUI_PAGE_INSTFILES
Page custom showExternalLinks
!insertmacro MUI_PAGE_FINISH

# Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

# Language files
!macro LANG_LOAD LANGLOAD
  !insertmacro MUI_LANGUAGE "${LANGLOAD}"
  !include "translations/${LANGLOAD}.nsh"
  !undef LANG
!macroend

!macro LANG_STRING NAME VALUE
  LangString "${NAME}" "${LANG_${LANG}}" "${VALUE}"
!macroend

!macro LANG_UNSTRING NAME VALUE
  !insertmacro LANG_STRING "un.${NAME}" "${VALUE}"
!macroend

# Put English first so that it'll be chosen on Windows versions whose
# language isn't supported.
!insertmacro LANG_LOAD "English"
!insertmacro LANG_LOAD "Basque"
!insertmacro LANG_LOAD "Bulgarian"
!insertmacro LANG_LOAD "Catalan"
!insertmacro LANG_LOAD "Czech"
!insertmacro LANG_LOAD "Dutch"
!insertmacro LANG_LOAD "French"
!insertmacro LANG_LOAD "German"
!insertmacro LANG_LOAD "Italian"
!insertmacro LANG_LOAD "Japanese"
!insertmacro LANG_LOAD "Korean"
!insertmacro LANG_LOAD "Lithuanian"
!insertmacro LANG_LOAD "Polish"
!insertmacro LANG_LOAD "Portuguese"
!insertmacro LANG_LOAD "PortugueseBR"
!insertmacro LANG_LOAD "Romanian"
!insertmacro LANG_LOAD "Russian"
!insertmacro LANG_LOAD "Serbian"
!insertmacro LANG_LOAD "SerbianLatin"
!insertmacro LANG_LOAD "Spanish"
!insertmacro LANG_LOAD "SimpChinese"
!insertmacro LANG_LOAD "Swedish"
!insertmacro LANG_LOAD "TradChinese"
!insertmacro LANG_LOAD "Turkish"
!insertmacro LANG_LOAD "Ukrainian"
!define MUI_LANGDLL_ALLLANGUAGES

!insertmacro MUI_RESERVEFILE_LANGDLL

# MUI end ------

!include "WinVer.nsh"
!include "LogicLib.nsh"


!if ${MINGW_PROCESSOR_ARCH} == "amd64"
  Name "${PRODUCT_NAME} ${PRODUCT_VERSION} (64-bit)${PRODUCT_VERSION_BUILD}"
  BrandingText "${PRODUCT_NAME} ${PRODUCT_VERSION} (64-bit)${PRODUCT_VERSION_BUILD} by ${PRODUCT_PUBLISHER}"
  OutFile "mkvtoolnix-64-bit-${PRODUCT_VERSION}-setup.exe"
  InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
!else
  Name "${PRODUCT_NAME} ${PRODUCT_VERSION} (32-bit)${PRODUCT_VERSION_BUILD}"
  BrandingText "${PRODUCT_NAME} ${PRODUCT_VERSION} (32-bit)${PRODUCT_VERSION_BUILD} by ${PRODUCT_PUBLISHER}"
  OutFile "mkvtoolnix-32-bit-${PRODUCT_VERSION}-setup.exe"
  InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
!endif

InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${PRODUCT_VERSION}${PRODUCT_VERSION_BUILD}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "${PRODUCT_NAME} is a set of tools to create, alter and inspect Matroska files under Linux, other Unices and Windows."
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${PRODUCT_PUBLISHER} ${PRODUCT_WEB_SITE}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_NAME} ${PRODUCT_VERSION}${PRODUCT_VERSION_BUILD}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}${PRODUCT_VERSION_BUILD}"

RequestExecutionLevel none

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Section "Program files" SEC01
  SetShellVarContext all

  SetOutPath "$INSTDIR"
  File "../*.exe"
  File "../*.url"
  File /r /x "portable-app" "../data"
  File /r "../doc"
  File /r "../examples"
  File /r "../locale"
  File /r "../tools"

  ; Both if Qt is linked shared and if Dr. MinGW is used:
  File /nonfatal "../*.dll"

!if ${MTX_LINK_TYPE} == "shared"
  File "../qt.conf"
  File /r "../plugins"
!endif

  # Delete files that might be present from older installation
  # if this is just an upgrade.
  Delete "$INSTDIR\mkv*.ico"
  Delete "$INSTDIR\mkvinfo-gui.exe"
  Delete "$INSTDIR\mmg.exe"
  Delete "$INSTDIR\data\magic.mgc"
  Delete "$INSTDIR\doc\command_line_references_and_guide.html"
  Delete "$INSTDIR\doc\en\mmg.html"
  Delete "$INSTDIR\doc\de\mmg.html"
  Delete "$INSTDIR\doc\es\mmg.html"
  Delete "$INSTDIR\doc\ja\mmg.html"
  Delete "$INSTDIR\doc\nl\mmg.html"
  Delete "$INSTDIR\doc\uk\mmg.html"
  Delete "$INSTDIR\doc\zh_CN\mmg.html"
  Delete "$INSTDIR\examples\example-timecodes-v1.txt"
  Delete "$INSTDIR\examples\example-timecodes-v2.txt"
  Delete "$INSTDIR\locale\cs\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\de\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\fr\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\it\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\ja\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\pl\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\ru\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\uk\LC_MESSAGES\qtbase.qm"
  Delete "$INSTDIR\locale\ca\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\cs\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\de\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\es\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\eu\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\fr\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\it\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\it\LC_MESSAGES\wxmsw.mo"
  Delete "$INSTDIR\locale\ja\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\nl\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\pl\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\ru\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\sv\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\tr\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\uk\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\zh_CN\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\locale\zh_TW\LC_MESSAGES\wxstd.mo"
  Delete "$INSTDIR\share\misc\magic.mgc"
  RMDir /r "$INSTDIR\doc\guide"
  RMDir /r "$INSTDIR\doc\images"
  RMDir /r "$INSTDIR\locale\rs"
  RMDir    "$INSTDIR\share\misc"
  RMDir    "$INSTDIR\share"

  # The docs have been moved to locale specific subfolders.
  Delete "$INSTDIR\doc\mkvextract.html"
  Delete "$INSTDIR\doc\mkvinfo.html"
  Delete "$INSTDIR\doc\mkvmerge.html"
  Delete "$INSTDIR\doc\mkvpropedit.html"
  Delete "$INSTDIR\doc\mmg.html"
  Delete "$INSTDIR\doc\mkvmerge-gui.*"

  Delete "$INSTDIR\doc\README.Windows.txt"
  Delete "$INSTDIR\doc\ChangeLog.txt"

  Delete "$SMPROGRAMS\$ICONS_GROUP\mkvmerge GUI.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MKVToolNix GUI preview.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Documentation\ChangeLog - What is new.lnk"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\AppMainExe.exe"

  Delete "$SMPROGRAMS\$ICONS_GROUP\Documentation\mkvmerge GUI guide.lnk"
  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation\mkvmerge GUI guide"
  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation\Other documentation"
  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation\Command line reference"

  SetShellVarContext current

  Delete "$SMPROGRAMS\$ICONS_GROUP\Documentation\mkvmerge GUI guide.lnk"
  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation\mkvmerge GUI guide"
  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation\Other documentation"
  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation\Command line reference"

  SetShellVarContext all

  # Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  SetOutPath "$INSTDIR"
  Delete "$SMPROGRAMS\$ICONS_GROUP\mkvinfo GUI.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MKVToolNix GUI preview.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Documentation\Command line references and guide.lnk"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MKVToolNix GUI.lnk" "$INSTDIR\mkvtoolnix-gui.exe" "" "$INSTDIR\mkvtoolnix-gui.exe"
  SetOutPath "$INSTDIR\Doc"
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP\Documentation"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Documentation\Command line references.lnk" "$INSTDIR\doc\command_line_references.html"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Documentation\NEWS.txt - What is new, what has changed.lnk" "$INSTDIR\doc\NEWS.txt"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Documentation\README.lnk" "$INSTDIR\doc\README.txt"
  !insertmacro MUI_STARTMENU_WRITE_END

  ${registerExtension} "$INSTDIR\mkvtoolnix-gui.exe" ".mtxcfg" "MKVToolNix GUI Settings"

  SetOutPath "$INSTDIR"
SectionEnd

Section -AdditionalIcons
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\mkvtoolnix-gui.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\mkvtoolnix-gui.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Function mediaFoundationFrameworkCheck
  IfFileExists $SYSDIR\evr.dll 0 +2
  Return

  IfSilent 0 +2
  Quit

  nsDialogs::Create 1018
  Pop $Dialog

  ${If} $Dialog == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 32u "$(STRING_MFF_NOT_FOUND)"
  Pop $Label1

  ${NSD_CreateLink} 0 32u 100% 16u "$(STRING_MFF_MORE_INFORMATION)"
  Pop $Link1

  ${NSD_CreateCheckBox} 0 48u 100% 16u "$(STRING_MFF_CONTINUE_ANYWAY)"
  Pop $CheckBox1

  ${NSD_OnClick} $Link1 onClickMediaFoundationMoreInformation

  nsDialogs::Show
FunctionEnd

Function mediaFoundationFrameworkCheckLeave
  ${NSD_GetState} $CheckBox1 $CheckBox1_State

  ${If} $CheckBox1_State == ${BST_UNCHECKED}
    Quit
  ${EndIf}
FunctionEnd

Function onClickMediaFoundationMoreInformation
  Pop $0
  ExecShell "open" "https://gitlab.com/mbunkus/mkvtoolnix/wikis/DLLs-not-found"
FunctionEnd

Function showExternalLinks
  IfSilent 0 +2
  Return

  nsDialogs::Create 1018
  Pop $Dialog

  ${If} $Dialog == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 32u "$(STRING_EXT_LINKS_INTRO)"
  Pop $Label1

  ${NSD_CreateLink} 0 32u 100% 16u "gMKVExtractGUI"
  Pop $Link1

  ${NSD_CreateLink} 0 48u 100% 16u "MKVcleaver"
  Pop $Link2

  ${NSD_OnClick} $Link1 onClickExternalLinkgMKVExtractGUI
  ${NSD_OnClick} $Link2 onClickExternalLinkMKVcleaver

  nsDialogs::Show
FunctionEnd

Function onClickExternalLinkgMKVExtractGUI
  Pop $0
  ExecShell "open" "https://sourceforge.net/projects/gmkvextractgui/"
FunctionEnd

Function onClickExternalLinkMKVcleaver
  Pop $0
  ExecShell "open" "https://blogs.sapib.ca/apps/mkvcleaver/"
FunctionEnd

var unRemoveJobs

Function un.onUninstSuccess
  HideWindow
  IfSilent +2 0
  MessageBox MB_ICONINFORMATION|MB_OK "$(STRING_UNINSTALLED_OK)"
FunctionEnd

Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
  IfSilent +3 0
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "$(STRING_REMOVE_PROGRAM_QUESTION)" IDYES +2
  Abort
  StrCpy $unRemoveJobs "No"
  IfFileExists "$INSTDIR\jobs\*.*" +2
  Return
  IfSilent +3 0
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "$(STRING_REMOVE_JOB_FILES_QUESTION)" IDYES +2
  Return
  StrCpy $unRemoveJobs "Yes"
FunctionEnd

Section Uninstall
  SetShellVarContext all

  ${unregisterExtension} ".mtxcfg" "MKVToolNix GUI Settings"

  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"

  Delete "$SMPROGRAMS\$ICONS_GROUP\mkvinfo GUI.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MKVToolNix GUI preview.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MKVToolNix GUI.lnk"

  RMDir /r "$SMPROGRAMS\$ICONS_GROUP\Documentation"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"

  Delete "$DESKTOP\mkvmerge GUI.lnk"
  Delete "$DESKTOP\MKVToolNix GUI preview.lnk"
  Delete "$DESKTOP\MKVToolNix GUI.lnk"

  Delete "$INSTDIR\MKVToolNix.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\mkvextract.exe"
  Delete "$INSTDIR\mkvinfo.exe"
  Delete "$INSTDIR\mkvinfo-gui.exe"
  Delete "$INSTDIR\mkvmerge.exe"
  Delete "$INSTDIR\mkvpropedit.exe"
  Delete "$INSTDIR\mkvtoolnix.exe"
  Delete "$INSTDIR\mkvtoolnix-gui.exe"
  Delete "$INSTDIR\lib*.dll"
  Delete "$INSTDIR\Qt5*.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\qt.conf"

  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\doc"
  RMDir /r "$INSTDIR\examples"
  RMDir /r "$INSTDIR\locale"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\share"
  RMDir /r "$INSTDIR\tools"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  DeleteRegKey HKLM "${MTX_REGKEY}"
  DeleteRegKey HKCU "${MTX_REGKEY}"

  SetAutoClose true
SectionEnd
