$project = $args[0]
$xcalbuild = "${PSScriptRoot}\..\..\out\install\bin\xcalbuild.exe"
$cwd=pwd

Set-Location "${PSScriptRoot}\..\..\tests\win-projects\$project"
$projectDir=pwd
if (Test-Path -Path "source-dir") {
	$fileContent=Get-Content source-dir
	$sourceDir="$projectDir/$fileContent"
} else {
	$sourceDir="$projectDir/src"
}

$buildCmd=Get-Content build-cmd
$prebuildCmd=Get-Content prebuild-cmd
$profile=Get-Content profile-name

mkdir src
Set-Location src

. $projectDir\configure.ps1

Set-Location $projectDir

$procArgs=@(
	"-i", "`"$sourceDir`"",  "-o", "`"$projectDir`"", "--profile", $profile, "-p", "`"$prebuildCmd`"", "--"
) + $buildCmd.Split([Environment]::NewLine)

Write-Host "`"$xcalbuild`" $procArgs"

$t = Measure-Command {& "$xcalbuild" @procArgs | Tee-Object -FilePath "$projectDir/build.log"}

# Needed for scripts to pickup time information
Add-Content -Path $projectDir/build.log -Value "xcalbuild_time@${t}"

Remove-Item $projectDir/src -Recurse -Force -Confirm:$false
Remove-Item $projectDir/preprocess -Recurse -Force -Confirm:$false