@echo off
setlocal
cd /d "%~dp0.."
echo.
echo LTA HARMONY - PUBLISH SOURCE TO GITHUB
where git >nul 2>nul
if errorlevel 1 (
  echo Git not found. Install Git for Windows, or use GitHub Desktop to publish this folder.
  pause
  exit /b 1
)
echo 1. First create an EMPTY PUBLIC repository on github.com/new
set /p REPO="2. Paste the NEW repository HTTPS URL ending .git: "
if "%REPO%"=="" (
  echo Repository URL cannot be blank.
  pause
  exit /b 1
)
git init || goto :fail
git add . || goto :fail
git -c user.name="LTA Harmony" -c user.email="lta-harmony@users.noreply.github.com" commit -m "Initial LTA Harmony VST3 source and GitHub Actions" || goto :fail
git branch -M main || goto :fail
git remote remove origin >nul 2>nul
git remote add origin "%REPO%" || goto :fail
git push -u origin main || goto :fail
echo.
echo Uploaded! Open your repository, go to Actions, then Build Windows VST3.
pause
exit /b 0
:fail
echo.
echo PUBLISH FAILED. Read the Git error above. You may need to sign in to GitHub.
pause
exit /b 1
