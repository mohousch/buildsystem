# how to build image: #

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

**for more details:**
```bash
$:~ make help
```

**supported boards:**
```bash
$:~ make print-boards
```

* backed image can be found into ~/buildsystem/tufsbox/$(machine)/image.

