#!/bin/env python3
import os
import sys
import shutil
import subprocess
import requests
import tarfile
import shutil
import concurrent.futures
import json
import glob
from datetime import datetime
'''
{package_name}_{version}-{revision}_{architecture}.deb
lib-llm_1.0-m5stack1_arm64.deb
llm-sys_1.0-m5stack1_arm64.deb
llm-audio_1.0-m5stack1_arm64.deb
llm-kws_1.0-m5stack1_arm64.deb
llm-asr_1.0-m5stack1_arm64.deb
llm-llm_1.0-m5stack1_arm64.deb
llm-tts_1.0-m5stack1_arm64.deb
'''

def create_bin_deb(package_name, version, src_folder, revision = 'm5stack1', depends = None):
    bin_files = glob.glob(os.path.join(src_folder, package_name.replace("-", "_") + "-*"))
    version_info = "0"
    if bin_files:
        for bin_file in bin_files:
            ver = bin_file.split('-')[-1]
            if ver > version_info:
                version_info = ver
        version = version_info

    deb_file = f"{package_name}_{version}-{revision}_arm64.deb"
    deb_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'debian-{}'.format(package_name))
    bin_path = 'usr/local/m5stack/bin'
    # os.makedirs(deb_folder, exist_ok=True)
    if os.path.exists(deb_folder):
        shutil.rmtree(deb_folder)
    print(f"Creating Debian package for {package_name} version {version}...: {os.path.join(deb_folder, bin_path)}")
    os.makedirs(os.path.join(deb_folder, bin_path), exist_ok = True)
    os.makedirs(os.path.join(deb_folder, 'DEBIAN'), exist_ok = True)
    # shutil.copytree(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'deb_overlay'), deb_folder)


    # if package_name == 'ec-proxy':
    #     llm_dir = os.path.join(src_folder, 'llm')
    #     if os.path.exists(llm_dir):
    #         shutil.copytree(llm_dir, os.path.join(deb_folder, 'opt/m5stack/lib/llm'))
    
    bin_file_name = package_name.replace("-", "_")
    if version_info != 0.0:
        bin_file_name = package_name.replace("-", "_") + f'-{version}'
    shutil.copy2(os.path.join(src_folder, bin_file_name), os.path.join(deb_folder, bin_path, bin_file_name))
    if package_name == 'ec-proxy':
        shutil.copy2(os.path.join(src_folder, bin_file_name).replace('proxy-', 'cli-'), os.path.join(deb_folder, bin_path, bin_file_name).replace('proxy-', 'cli-'))
    ext_scripts_files = glob.glob(os.path.join(src_folder, package_name + "_*"))
    if ext_scripts_files:
        os.makedirs(os.path.join(deb_folder, 'opt/m5stack/scripts'), exist_ok = True)
        for ext_script_file in ext_scripts_files:
            shutil.copy2(ext_script_file, os.path.join(deb_folder, 'opt/m5stack/scripts'))

    with open(os.path.join(deb_folder, 'DEBIAN/control'),'w') as f:
        f.write(f'Package: {package_name}\n')
        f.write(f'Version: {version}\n')
        f.write(f'Architecture: arm64\n')
        f.write(f'Maintainer: dianjixz <dianjixz@m5stack.com>\n')
        f.write(f'Original-Maintainer: m5stack <m5stack@m5stack.com>\n')
        f.write(f'Section: llm-module\n')
        f.write(f'Priority: optional\n')
        # f.write(f'Depends: lib-llm\n')
        if depends is not None: 
            f.write(f'Depends: {depends}\n')
        f.write(f'Homepage: https://www.m5stack.com\n')
        f.write(f'Packaged-Date: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}\n')
        f.write(f'Description: llm-module\n')
        f.write(f' bsp.\n')
    with open(os.path.join(deb_folder, 'DEBIAN/preinst'),'w') as f:
        f.write(f'#!/bin/sh\n')
        f.write(f'[ -f "/lib/systemd/system/ec_proxy.service" ] && systemctl stop ec_proxy.service\n')
        f.write(f'[ -f "/lib/systemd/system/ec_proxy.service" ] && systemctl disable ec_proxy.service\n')
        f.write(f'[ -f "/lib/systemd/system/ec_proxy.service" ] && rm -f /lib/systemd/system/ec_proxy.service\n')
        f.write(f'[ -f "/usr/local/m5stack/bin/ec_proxy-1.0" ] && rm -f /usr/local/m5stack/bin/ec_proxy-1.0\n')
        f.write(f'rm -f /bin/ec_cli\n')
        f.write(f'[ -f "/usr/local/m5stack/bin/ec_cli-1.0" ] && rm -f /usr/local/m5stack/bin/ec_cli-1.0\n')
        f.write(f'exit 0\n')

    with open(os.path.join(deb_folder, 'DEBIAN/postinst'),'w') as f:
        f.write(f'#!/bin/sh\n')
        f.write(f'[ -f "/lib/systemd/system/{package_name}.service" ] && systemctl enable {package_name}.service\n')
        f.write(f'[ -f "/lib/systemd/system/{package_name}.service" ] && systemctl start {package_name}.service\n')
        f.write(f'ln -s {os.path.join("/",bin_path, bin_file_name).replace("proxy-", "cli-")} /bin/ec_cli\n')
        f.write(f'exit 0\n')
    with open(os.path.join(deb_folder, 'DEBIAN/prerm'),'w') as f:
        f.write(f'#!/bin/sh\n')
        f.write(f'[ -f "/lib/systemd/system/{package_name}.service" ] && systemctl stop {package_name}.service\n')
        f.write(f'[ -f "/lib/systemd/system/{package_name}.service" ] && systemctl disable {package_name}.service\n')
        f.write(f'exit 0\n')
    os.makedirs(os.path.join(deb_folder, 'lib/systemd/system'), exist_ok = True)
    with open(os.path.join(deb_folder, f'lib/systemd/system/{package_name}.service'),'w') as f:
        f.write(f'[Unit]\n')
        f.write(f'Description={package_name} Service\n')
        f.write(f'\n')
        f.write(f'[Service]\n')
        f.write(f'ExecStart={os.path.join("/",bin_path, bin_file_name)}\n')
        f.write(f'WorkingDirectory={os.path.join("/",bin_path)}\n')
        f.write(f'Restart=always\n')
        f.write(f'RestartSec=1\n')
        f.write(f'StartLimitInterval=0\n')
        f.write(f'\n')
        f.write(f'[Install]\n')
        f.write(f'WantedBy=multi-user.target\n')
        f.write(f'\n')

    os.chmod(os.path.join(deb_folder, 'DEBIAN/preinst'), 0o755)
    os.chmod(os.path.join(deb_folder, 'DEBIAN/postinst'), 0o755)
    os.chmod(os.path.join(deb_folder, 'DEBIAN/prerm'), 0o755)

    subprocess.run(["dpkg-deb", "-b", deb_folder, deb_file], check=True)
    print(f"Debian package created: {deb_file}")
    shutil.rmtree(deb_folder)
    return package_name + " creat success!"

if __name__ == "__main__":

    if "clean" in sys.argv:
        os.system('rm ./*.deb')
        os.system('find . -maxdepth 1 -type d ! -name "." -exec rm -rf {} +')
        exit(0)
    if "distclean" in sys.argv:
        os.system('rm ./*.deb m5stack_* -rf')
        exit(0)

    version = '1.5'
    data_version = '0.2'
    src_folder = '../dist'
    revision = 'm5stack1'

    create_bin_deb('ec-proxy', '1.1', src_folder, revision)