#!perl

#    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.
#
#    This file is part of The Soul Ride Engine, see http://soulride.com
#
#    The Soul Ride Engine is free software; you can redistribute it
#    and/or modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version 2 of
#    the License, or (at your option) any later version.
#
#    The Soul Ride Engine is distributed in the hope that it will be
#    useful, but WITHOUT ANY WARRANTY; without even the implied
#    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with The Soul Ride Engine; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Script to generate .nsi file to feed into MakeNSIS, to make a
# self-extracting installer.

# First arg is the output filename, e.g. "virtual_stratton_1_0c.exe"
# Second arg is the name of the program, e.g. "Virtual Stratton"
# Third arg is ... path of files?

use strict;


my ($outfile, $mountain_name, $website, $program_name, $basepath, $filelist, $special_version) = @ARGV;

if ( $outfile eq ''
	 || $program_name eq ''
	 || $mountain_name eq ''
	 || $filelist eq ''
	 )
{
	print "Usage: gen_install.pl <outfile> \"mountain name\" \"program name\" <basepath> <filelist>\n";
	exit(1);
}

if ( $basepath eq '' ) {
	$basepath = "./";
}


my $readme_file = "readme-$mountain_name.txt";
my $default_mountain_flag = "DefaultMountain=$mountain_name";
if ($mountain_name eq "Soul_Ride")
{
	# Soul_Ride as a mountain name is a special case; no different
	# readme or DefaultMountain flag.
	$readme_file = "readme.txt";
	$default_mountain_flag = "";
}


my $extra_path = '';
if ($special_version eq 'Purplehills')
{
	$extra_path = "Purplehills\\";
}


# Get the list of files to include, and massage them into the format that
# MakeNSIS wants.
my $install_files;
my $uninstall_files;
my @files;
my %file_hash;
my $current_path = '---';
open IN, $filelist;
while (<IN>) {
	chomp($_);
	my $file = $_;

	if ($file ne '' && $file_hash{$file} != 1) {
		$file_hash{$file} = 1;  # Avoid installing duplicates.

		$file =~ /^(.*)\/[^\/]*$/;  # Get the path part of the filespec.
		my $path = $1;

		if ( $path ne $current_path ) {
			$install_files = $install_files . "SetOutPath \"\$INSTDIR\\$path\"\n";
			$current_path = $path;
		}
		$install_files = $install_files . "File \"$basepath$file\"\n";
		
		$uninstall_files = $uninstall_files . "Delete \"\$INSTDIR\\$file\"\n";
	}
}
close IN;

$install_files =~ s/\//\\/g;  # Replace /'s with \'s
$uninstall_files =~ s/\//\\/g;  # Replace /'s with \'s

print <<EOQ

;;
;; Install Script, adapted from NSIS sample "NSIS Modern User Interface version 1.63" Written by Joost Verburg
;;

;; !define MUI_PRODUCT "$program_name"
;; !define MUI_VERSION "X"

!include "MUI.nsh"

;--------------------------------
;Configuration

  ;General
  Name "$program_name"
  OutFile "$outfile"

;  ;Folder selection page
  InstallDir "\$PROGRAMFILES\\${extra_path}Soul Ride"
  
  ;Remember install folder
;  InstallDirRegKey HKCU "Software\\Slingshot Game Technology\\\${MUI_PRODUCT}" ""
  InstallDirRegKey HKLM "Software\\Slingshot Game Technology\\$program_name" ""
  

;--------------------------------
;Modern UI Configuration

;  !define MUI_LICENSEPAGE
;  !define MUI_COMPONENTSPAGE
  !define MUI_DIRECTORYPAGE
  
  !define MUI_ABORTWARNING
  
  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE


;--------------------------------
;Languages


  !insertmacro MUI_LANGUAGE "English"
;  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
;  !insertmacro MUI_LANGUAGE "SimpChinese"
;  !insertmacro MUI_LANGUAGE "TradChinese"    
;  !insertmacro MUI_LANGUAGE "Japanese"
;  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Italian"
;  !insertmacro MUI_LANGUAGE "Dutch"
;  !insertmacro MUI_LANGUAGE "Danish"
;  !insertmacro MUI_LANGUAGE "Greek"
;  !insertmacro MUI_LANGUAGE "Russian"
;  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Polish"
;  !insertmacro MUI_LANGUAGE "Ukrainian"
;  !insertmacro MUI_LANGUAGE "Czech"
;  !insertmacro MUI_LANGUAGE "Slovak"
;  !insertmacro MUI_LANGUAGE "Croatian"
;  !insertmacro MUI_LANGUAGE "Bulgarian"
;  !insertmacro MUI_LANGUAGE "Hungarian"
;  !insertmacro MUI_LANGUAGE "Thai"
;  !insertmacro MUI_LANGUAGE "Romanian"
;  !insertmacro MUI_LANGUAGE "Macedonian"
;  !insertmacro MUI_LANGUAGE "Turkish"
  
;--------------------------------
;Language Strings
    
  ;Descriptions
  LangString DESC_SecCopyUI \${LANG_ENGLISH} "modern.exe: English description"
  LangString DESC_SecCopyUI \${LANG_FRENCH} "modern.exe: French description"
  LangString DESC_SecCopyUI \${LANG_GERMAN} "modern.exe: German description"
  LangString DESC_SecCopyUI \${LANG_SPANISH} "modern.exe: Spanish description"
  LangString DESC_SecCopyUI \${LANG_SIMPCHINESE} "modern.exe: Simplified Chinese description"
  LangString DESC_SecCopyUI \${LANG_TRADCHINESE} "modern.exe: Traditional Chinese description"
  LangString DESC_SecCopyUI \${LANG_JAPANESE} "modern.exe: Japanese description"
  LangString DESC_SecCopyUI \${LANG_KOREAN} "modern.exe: Korean description"
  LangString DESC_SecCopyUI \${LANG_ITALIAN} "modern.exe: Italian description"
  LangString DESC_SecCopyUI \${LANG_DUTCH} "modern.exe: Dutch description"
  LangString DESC_SecCopyUI \${LANG_DANISH} "modern.exe: Danish description"
  LangString DESC_SecCopyUI \${LANG_GREEK} "modern.exe: Greek description"
  LangString DESC_SecCopyUI \${LANG_RUSSIAN} "modern.exe: Russian description"
  LangString DESC_SecCopyUI \${LANG_PORTUGUESEBR} "modern.exe: Portuguese (Brasil) description"
  LangString DESC_SecCopyUI \${LANG_POLISH} "modern.exe: Polish description"
  LangString DESC_SecCopyUI \${LANG_UKRAINIAN} "modern.exe: Ukrainian description"
  LangString DESC_SecCopyUI \${LANG_CZECH} "modern.exe: Czechian description"
  LangString DESC_SecCopyUI \${LANG_SLOVAK} "modern.exe: Slovakian description"
  LangString DESC_SecCopyUI \${LANG_CROATIAN} "modern.exe: Slovakian description"
  LangString DESC_SecCopyUI \${LANG_BULGARIAN} "modern.exe: Bulgarian description"
  LangString DESC_SecCopyUI \${LANG_HUNGARIAN} "modern.exe: Hungarian description"
  LangString DESC_SecCopyUI \${LANG_THAI} "modern.exe: Thai description"
  LangString DESC_SecCopyUI \${LANG_ROMANIAN} "modern.exe: Romanian description"
  LangString DESC_SecCopyUI \${LANG_MACEDONIAN} "modern.exe: Macedonian description"
  LangString DESC_SecCopyUI \${LANG_TURKISH} "modern.exe: Turkish description"
  
;--------------------------------
;Reserve Files
  
  ;Things that need to be extracted on first (keep these lines before any File command!)
  ;Only useful for BZIP2 compression
  !insertmacro MUI_RESERVEFILE_LANGDLL


;--------------------------------
; misc config

LangString SRLicenseText \${LANG_ENGLISH} "$program_name Copyright 2003 Slingshot Game Technology <http://soulride.com>"
LangString SRLicenseText \${LANG_GERMAN} "$program_name Copyright 2003 Slingshot Game Technology <http://soulride.com>"
LangString SRLicenseText \${LANG_POLISH} "$program_name Copyright 2003 Slingshot Game Technology <http://soulride.com>"
LangString SRLicenseText \${LANG_SPANISH} "$program_name Copyright 2003 Slingshot Game Technology <http://soulride.com>"
LangString SRLicenseText \${LANG_ITALIAN} "$program_name Copyright 2003 Slingshot Game Technology <http://soulride.com>"
LicenseText \$(SRLicenseText)

CRCCheck on ; (can be off)

ShowInstDetails show ; (hide to hide; show to have them shown, or nevershow to disable)
SetDateSave on ; (default off; can be on to have files restored to their orginal date)
SetOverwrite ifnewer ; [on/off/try/ifnewer]

;--------------------------------
; main section

Section "" ; (default section)
	; add files / whatever that need to be installed here.
	; File /r c:/path/to/files/*.*	; recursively adds all the files in the dir
	$install_files
	
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\\Slingshot Game Technology\\$program_name" "" "\$INSTDIR"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\$program_name" "DisplayName" "$program_name (remove only)"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\$program_name" "UninstallString" '"\$INSTDIR\\uninst.exe"'
	
	
	SetOutPath "\$SMPROGRAMS\\${extra_path}$program_name"
	SetOutPath "\$INSTDIR"


;; Show the language...
;;	    MessageBox MB_OK|MB_ICONEXCLAMATION "LANGUAGE = \$LANGUAGE"
	
	;---- language-specific setup of links ----
	StrCpy \$R1 "Language=en"
	StrCpy \$R2 "$readme_file"
	StrCpy \$R3 "Run $program_name"
	StrCpy \$R4 "Slingshot web site"
	StrCpy \$R5 "http://www.soulride.com"
	StrCpy \$R6 "$program_name web site"
	StrCpy \$R7 "$website"
	StrCpy \$R8 "Uninstall $program_name"

	;; Get the base language, with the variant masked off.
	IntFmt \$0 "%u" \$LANGUAGE
	IntOp \$0 \$0 & 255

	IntCmpU \$0 7 lang_de
	IntCmpU \$0 21 lang_pl
	IntCmpU \$0 16 lang_it
	;; IntCmpU \$0 12 lang_fr
	;; etc.
	goto end_language_check

 lang_de:
	StrCpy \$R1 "Language=de"
	StrCpy \$R2 "readme-de.txt"
	StrCpy \$R3 "$program_name starten"
	StrCpy \$R6 "$program_name Website"
	StrCpy \$R8 "Deinstallation $program_name"
	StrCmp "$program_name" "Soul Ride" purplehills_stuff end_language_check
 purplehills_stuff:
	;; Extra purplehills stuff
	StrCpy \$R2 ""
	StrCpy \$R4 "Purplehills Website"
	StrCpy \$R5 "http://www.purplehills.de/soulride"
	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\$program_name Hilfe.lnk" \\
					"\$INSTDIR\\srmanual-de.pdf"
	goto end_language_check

 lang_pl:
	StrCpy \$R1 "Language=pl"
	StrCpy \$R2 "czytajto.txt"
	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\Instrukcja.lnk" \\
					"\$INSTDIR\\Instrukcja\\instrukcja.html"
	goto end_language_check

 lang_it:
	StrCpy \$R1 "Language=it"
	StrCpy \$R2 "readme.txt"
;;	StrCpy \$R3 "$program_name starten"
;;	StrCpy \$R6 "$program_name Website"
;;	StrCpy \$R8 "Deinstallation $program_name"
	goto end_language_check

 ;;lang_fr:
	;; 	StrCpy \$R1 "Language=fr"
	;; 	goto end_language_check
	;; etc for supported languages
 end_language_check:


	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\\$R6.lnk" \\
	                 \$R7
	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\\$R4.lnk" \\
	                 \$R5
	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\\$R3.lnk" \\
	                 "\$INSTDIR\\soulride.exe" "$default_mountain_flag \$R1"

	StrCmp \$R2 "" no_readme
	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\Readme.lnk" \\
	                 "\$INSTDIR\\\$R2"
 no_readme:
	CreateShortCut "\$SMPROGRAMS\\${extra_path}$program_name\\\$R8.lnk" \\
	                 "\$INSTDIR\\uninst.exe"
	
	WriteUninstaller "uninst.exe"

SectionEnd ; end of default section


;Display the Finish header
;Insert this macro after the sections if you are not using a finish page
;; !insertmacro MUI_SECTIONS_FINISHHEADER

;--------------------------------
;Installer Functions

Function .onInstSuccess
	; Show the program group after installing.
	ExecShell open "\$SMPROGRAMS\\${extra_path}$program_name"
FunctionEnd


;--------------------------------
;Uninstaller Section

;; UninstallText "This will uninstall $program_name from your system"

Section Uninstall

	; add delete commands to delete whatever files/registry keys/etc you installed here.
	
	$uninstall_files
	
	; RMDir /r \$INSTDIR\\stuff
	; RMDir \$INSTDIR

;; Just delete it.
;;	; if \$INSTDIR was removed, skip these next ones
;;	IfFileExists \$INSTDIR 0 Removed 
;;	  MessageBox MB_YESNO|MB_ICONQUESTION \\
;;	    "Remove all files in your $program_name directory? (If you have anything you created that you want to keep, click No)" IDNO Removed
;;	  Delete \$INSTDIR\\*.* ; this would be skipped if the user hits no
	  RMDir /r \$INSTDIR
;;	  IfFileExists \$INSTDIR 0 Removed 
;;	    MessageBox MB_OK|MB_ICONEXCLAMATION \\
;;	               "Note: \$INSTDIR could not be removed."
;;	Removed:
	
	
	Delete "\$INSTDIR\\uninst.exe"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\\Slingshot Game Technology\\$program_name"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\$program_name"
	RMDir "\$INSTDIR"
	RMDir /r "\$SMPROGRAMS\\${extra_path}$program_name"
	RMDir "\$SMPROGRAMS\\${extra_path}"

	;Display the Finish header
	;!insertmacro MUI_UNFINISHHEADER
	;; MUI_UNPAGE_FINISH ?

SectionEnd ; end of uninstall section


EOQ
