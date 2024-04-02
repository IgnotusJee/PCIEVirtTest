# PCIEVirtTest

尝试通过借鉴 NVMeVirt 项目，编写一个可被内核识别为虚拟 PCIE 设备的内核模块。

目前进度：剔除了NVMe的部分，PCIE的部分写上了注释，能正常装载卸载，lspci能识别出设备类型

![alt text](<wallpaper.jpg>)