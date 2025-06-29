# How to build image: #

```bash
$:~ git clone https://github.com/mohousch/buildsystem.git
```

```bash
cd buildsystem
```

**for first use:**
```bash
$:~ sudo bash prepare-for-bs.sh
```

**machine configuration:**
```bash
$:~ make
```

or

```bash
$:~ make init
```

**build image:**
```bash
$:~ make image-neutrino2
```

if you want to build neutrino-DDT image

```bash
$:~ make image-neutrino
```

**For more details:**
```bash
$:~ make help
```

**Supported boards:**
```bash
$:~ make print-boards
```

* Backed image can be found into ~/buildsystem/tufsbox/$(machine)/image.

# DISCLAIMER
* Tested STBs:
- Gigablue UE 4K
- Gigablue Ultra UE
- Gigablue 800se
- WWIO Bre2zt2c
- Cuberevo Mini2 (3000HD)

* All others platforms are not tested, do it on your own risk, if you have request or issue please post it here.



