# Quick Reference: Getting Started

## Step-by-Step Setup (5 minutes)

### Step 1: Create GitHub Repository
```
1. Visit https://github.com/new
2. Name: sup_new_bbcode
3. Public (optional: Private)
4. Leave all checkboxes unchecked
5. Click "Create repository"
6. Copy the repository URL
```

### Step 2: First Push
Open PowerShell and run these commands:

```powershell
cd L:\SUP_BB_Code_New

git config user.email "your-email@example.com"
git config user.name "Your Name"

git init
git add .
git commit -m "Initial commit: StreamUP BBCode Plugin"

# Replace YOUR_USERNAME with your GitHub username
git remote add origin https://github.com/YOUR_USERNAME/sup_new_bbcode.git
git branch -M main
git push -u origin main
```

That's it! Your repository is now set up with automated builds.

## Files Created for You

- `.github/workflows/build.yml` - Automated build configuration
- `push-to-new-repo.ps1` - Helper script to push to new repo
- `SETUP_AUTOMATED_BUILDS.md` - Full setup guide
- This file - Quick reference

## What Happens Next

✅ Every time you `git push`, builds automatically start
✅ See build status in GitHub repository "Actions" tab
✅ Three platforms built: Windows, macOS, Linux
✅ Artifacts uploaded after each build

## Update Your Code Workflow

```powershell
# Make changes to files...

# Then run:
git add .
git commit -m "Your change description"
git push

# Builds start automatically! Check Actions tab to watch.
```

## Create a Release

```powershell
git tag -a v0.1.1 -m "Release v0.1.1"
git push origin v0.1.1
```

This creates a GitHub Release with compiled binaries attached.

## Troubleshooting First Push

**Error: "repository not found"**
- Double-check GitHub username in URL
- Verify repository exists at https://github.com/YOUR_USERNAME/sup_new_bbcode

**Error: "permission denied"**
- Ensure GitHub credentials are configured
- Run: `git config --list`

**Already have origin?**
```powershell
git remote remove origin
git remote add origin https://github.com/YOUR_USERNAME/sup_new_bbcode.git
```
