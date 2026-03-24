# Setup Guide: Automated Builds and New Repository

## Quick Start

### 1. Create the New Independent Repository

1. Go to **https://github.com/new**
2. Repository name: **sup_new_bbcode**
3. Description: StreamUP - BBCode Macros Plugin
4. **Choose "Public"** (or "Private" if preferred)
5. **Leave all initialization options unchecked** (no README, .gitignore, or license)
6. Click **Create repository**

### 2. Initialize Local Git Repository

Open PowerShell in `L:\SUP_BB_Code_New` and run:

```powershell
cd L:\SUP_BB_Code_New

# Initialize git (if not already done)
git init
git config user.email "your-email@example.com"
git config user.name "Your Name"

# Add all files
git add .

# Create initial commit
git commit -m "Initial commit: StreamUP BBCode Plugin"
```

### 3. Push to New Repository

```powershell
# Replace YOUR_USERNAME with your actual GitHub username
.\push-to-new-repo.ps1 -GitHubUsername "YOUR_USERNAME"
```

Or manually:

```powershell
git remote add origin https://github.com/YOUR_USERNAME/sup_new_bbcode.git
git branch -M main
git push -u origin main
```

## Automated Builds

The GitHub Actions workflow (`.github/workflows/build.yml`) will automatically:

✅ **Build on every push** to `main` or `develop` branches
✅ **Build on every pull request**
✅ **Compile for Windows, macOS, and Linux**
✅ **Generate build artifacts** for each platform
✅ **Create releases** when you tag a version

### Creating a Release Build

To create a release that builds and publishes artifacts:

```powershell
# Tag a new version
git tag -a v0.1.0 -m "Release version 0.1.0"

# Push the tag (this triggers the release workflow)
git push origin v0.1.0
```

The workflow will:
1. Build all three platforms
2. Create a GitHub Release
3. Attach compiled binaries to the release

## Updating the Code

Whenever you make changes:

```powershell
# Stage your changes
git add .

# Commit with a descriptive message
git commit -m "Add feature: description here"

# Push to repository (builds automatically start!)
git push
```

The build status will be visible:
- In your GitHub repository under the "Actions" tab
- As a checkmark or X next to your commits
- In pull request status checks

## Customizing Builds

Edit `.github/workflows/build.yml` to:
- Change build configurations
- Add more platforms
- Add code quality checks
- Deploy artifacts automatically

## Repository Settings

Recommended GitHub settings:
1. Go to your repository settings
2. **Branch protection rules**: Protect `main` branch
3. **Require status checks**: Require builds to pass before merging
4. **Require pull request reviews**: Ensure code review before merge

## Common Commands

```powershell
# Check git status
git status

# View commit history
git log --oneline

# Create a new branch for features
git checkout -b feature/my-feature

# Switch back to main
git checkout main

# View all branches
git branch -a
```

## Troubleshooting

**Build fails on Windows?**
- Ensure Visual Studio 2022 build tools are installed
- Check CMake version: `cmake --version`

**Push fails?**
- Verify you have GitHub credentials configured
- Check: `git config --global user.name`

**Artifacts not uploading?**
- Check the build succeeded in the Actions tab
- Verify artifact paths in `.github/workflows/build.yml`

Need help? Check the build logs in your GitHub repository's **Actions** tab.
