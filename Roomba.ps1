#REQUIRES -Version 3.0

[CmdletBinding()]
Param(
	[Parameter(Mandatory=$false)]
	[Switch]
	$FullClean,
	
	[Parameter(Mandatory=$false)]
	[Switch]
	$CleanConfigsAndSaves
)

if ($CleanConfigsAndSaves -and -not $FullClean)
{
	throw "-CleanConfigsAndSaves can only be specified when -FullClean is also specified."
}

$uprojectFile = Get-ChildItem -Path "$PSScriptRoot" -Filter *.uproject -ErrorAction SilentlyContinue

if ($null -eq $uprojectFile)
{
	throw "Could not find .uproject file in folder"
}

$uprojectName = $uprojectFile.BaseName;

Write-Output "Cleaning up"

if ($CleanConfigsAndSaves)
{
    Write-Output "Cleaning configs and saves is an extreme option that MIGHT solve the issue only in very specific circumstances. You might want to backup those directories before proceeding. Proceed with cleanup? (Press Y to continue, N to break)"
	
	do    
	{
		$input = [System.Console]::ReadKey("NoEcho").KeyChar
    }
	until ($input -ieq 'y' -or $input -ieq 'n')
    
	if ($input -ieq 'n')
	{
		Write-Output "Interrupting cleanup"
		return
	}
}

$directoriesToDelete = @(
	"Binaries",
	"Intermediate",
	"Saved\Cooked",
	"Saved\StagedBuilds",
	"Saved\Temp"
	)
	
if ($FullClean)
{
	$directoriesToDelete += @(
		"DerivedDataCache",
		"Saved\EditorCooked",
		"Saved\Shaders"
		)
		
	if ($CleanConfigsAndSaves)
	{
		$directoriesToDelete += @(
			"Saved\Config",
			"Saved\SaveGames"
		)
	}
}

$directoriesToDelete = $directoriesToDelete | ForEach-Object { Get-Item -Path "$PSScriptRoot\$_" -ErrorAction SilentlyContinue }
$filesToDelete = Get-ChildItem -Path "$PSScriptRoot\Saved" -Recurse -Filter *.tmp -ErrorAction SilentlyContinue

foreach ($directory in $directoriesToDelete)
{
	if($directory)
	{
    	Write-Output $directory.FullName
		Remove-Item -Path $directory.FullName -Recurse -Force
	}
}
foreach ($file in $filesToDelete) 
{
	if($file) 
	{
		Write-Output $file.FullName
		Remove-Item -Path $file.FullName -Force
	}
}

Write-Output "Generating VS project files"
Write-Output "$PSScriptRoot/$uprojectName.uproject"

$versionSelectorPath = $(Get-ItemProperty 'Registry::HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj').Icon
$fullCommand = "& $versionSelectorPath /projectfiles $PSScriptRoot/$uprojectName.uproject"

Invoke-Expression -command $fullCommand
