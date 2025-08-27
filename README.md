# How to build image: #

```bash
$:~ git clone https://github.com/mohousch/NeutrinoNG.git
```

```bash
cd NeutrinoNG
```

**for first use:**
```bash
$:~ sudo bash prepare-for-bs.sh
```

**machine configuration:**
```bash
$:~ make init
```

or

```bash
$:~ make
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

* Backed image can be found into ~/NeutrinoNG/tufsbox/$(machine)/image.

# DISCLAIMER
* Tested on:
- Gigablue UE 4K
- Gigablue Ultra UE
- Gigablue 800se
- WWIO Bre2zt2c
- Cuberevo 3000HD / IPBox 910HD

* All others platforms are not tested, do it on your own risk.



