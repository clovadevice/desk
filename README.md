# DESK Opensource 

# How to build

0. Setup build environment
   - please refer to https://source.android.com/source/initializing for the build environment.
   - setup repo
     ```
     mkdir ~/bin
     PATH=~/bin:$PATH
     curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
     chmod a+x ~/bin/rep
     ```
  
1. Get Android open source from CodeAurora and DESK kernel
   - android version: 7.1.2  
   ```
   repo init -u https://source.codeaurora.org/quic/la/platform/manifest.git -b release -m LA.UM.5.6.r1-08900-89xx.0.xml --repo-url=git://codeaurora.org/tools/repo.git --repo-branch=caf-stable
   ```

2. Download the kernel
   ```
   git clone https://github.com/clovadevice/desk.git
   ```

3. Build
   ```
   cd kernel
   make ARCH=arm if_s700n_defconfig
   make mrproper
   make bootimage   
   ```

# License
Please see licenses/NOTICE.html, licenses/NOTICE_line.txt for the full open source license.