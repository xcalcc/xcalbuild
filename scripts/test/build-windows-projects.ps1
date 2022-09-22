$files = Get-ChildItem win-projects

foreach($f in $files) {
	& .\run-project.ps1 $f.Name
}

tar -cvzf windows-run.tar.gz .