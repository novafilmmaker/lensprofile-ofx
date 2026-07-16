# LensProfileOFX — Fase 1: Aberración cromática

Primer efecto funcional de punta a punta del motor de perfiles ópticos (ver
`arquitectura-ofx-plugin-opticas.md` y `hoja-de-ruta-fases.md`).

## Qué hace

Un único efecto OFX, "Lens Profile - Aberracion Cromatica", que:

- Lee los coeficientes de aberración cromática de `profiles/canon_k35_50mm.json`.
- Los escala con un slider "Carácter" (0–150%, 100% = perfil tal cual).
- Renderiza en GPU vía Metal (con respaldo CPU multihilo si Metal no está disponible).
- Procesa en float32 (lineal, alta profundidad de bits).

## Cómo instalarlo — 3 caminos, según quién sos

### No sabés programar / no querés usar Terminal (Recomendado para vos)

Ver **`GUIA-INSTALADOR-SIN-CODIGO.md`** (en la raíz del proyecto que te
compartí, un nivel arriba de esta carpeta). Es 100% clics: subís el código a
una cuenta gratuita de GitHub, un robot lo compila automáticamente en la
nube, y descargás un instalador `.pkg` clásico de macOS (doble clic,
Continuar → Instalar → listo). No abrís Terminal en ningún momento.

### Sos vos iterando sobre el código (sabés algo de Terminal)

`install.sh` es un instalador guiado: revisa requisitos, compila, e instala
paso a paso.

```bash
cd fase1-aberracion-cromatica
chmod +x install.sh uninstall.sh
./install.sh
```

Flags útiles: `./install.sh -y` salta confirmaciones, `./install.sh --uninstall`
desinstala.

### Querés generar el .pkg vos mismo en tu Mac (sin pasar por GitHub)

Después de compilar una vez con `install.sh`:

```bash
cd fase1-aberracion-cromatica/installer
chmod +x build_installer.sh
./build_installer.sh
```

Esto genera `LensProfileOFX-Installer.pkg` en la raíz del proyecto — el
mismo instalador clásico que produce el camino de GitHub Actions, pero
generado localmente.

## Probarlo en Resolve

1. Abrí un proyecto de prueba, importá un clip cualquiera.
2. Andá al panel **Effects** → **OpenFX** → grupo **Bitacora Films**.
3. Arrastrá "Lens Profile - Aberracion Cromatica" sobre el clip en el timeline.
4. Deberías ver franjas de color (rojo/azul) corriéndose hacia los bordes de
   la imagen, más marcado en las esquinas. El slider "Caracter" escala esto.

Si el efecto no aparece en el panel:

- Confirmá que el bundle está en `/Library/OFX/Plugins/LensProfileOFX.ofx.bundle`
  (no en `~/Library/...`, tiene que ser la Library de raíz del sistema).
- Reiniciá Resolve completamente (Resolve → Quit, no solo cerrar el proyecto).
- Resolve escribe logs de carga de plugins OFX en su consola interna — ahí
  suele aparecer el motivo si el bundle no carga (símbolo faltante, Info.plist
  mal formado, etc.).

## Qué NO se pudo verificar en este entorno

Este código se escribió y revisó cuidadosamente contra la documentación real
del OpenFX SDK y ejemplos publicados de plugins OFX/Metal para Resolve (ver
notas en los comentarios de `MetalKernel.mm`), pero **no se compiló ni se
corrió acá**, porque este entorno de trabajo es Linux sin Xcode/Metal — Apple
no permite compilar ni ejecutar Metal fuera de macOS. Lo único que sí se
probó de punta a punta fue la lógica de `ProfileLoader` (parseo del JSON de
perfiles), que no depende de OFX ni de Metal — ver `tests/test_profile_loader.cpp`.
También se verificó la sintaxis de todos los scripts (`bash -n`) y que
`Distribution.xml` sea XML válido, pero `pkgbuild`/`productbuild` en sí son
herramientas de Apple que tampoco corrieron acá.

Por eso existe el camino de GitHub Actions: compila en una Mac real (virtual,
pero real) cada vez, así que cualquier error de compilación real aparece ahí
y se puede corregir sin que dependas de tener vos una Mac con Xcode a mano.

## Estructura

```
fase1-aberracion-cromatica/
├── CMakeLists.txt          # build system (descarga OpenFX SDK + nlohmann/json solo)
├── Info.plist              # metadata del bundle macOS
├── osxSymbols               # lista de símbolos exportados (requisito de OFX en macOS)
├── install.sh               # instalador guiado: compila + instala
├── uninstall.sh              # desinstala
├── .github/workflows/build.yml  # compila y publica el .pkg automaticamente en GitHub
├── installer/
│   ├── build_installer.sh    # genera el .pkg localmente
│   ├── Distribution.xml      # define las pantallas del asistente del .pkg
│   ├── resources/
│   │   ├── welcome.txt       # pantalla de bienvenida del instalador
│   │   └── readme.txt        # pantalla de "léame" del instalador
│   └── scripts/
│       └── postinstall       # corre al final de la instalación del .pkg
├── profiles/
│   └── canon_k35_50mm.json  # perfil de prueba (coeficientes no calibrados aún)
├── src/
│   ├── LensProfile.h              # struct de datos de un perfil
│   ├── ProfileLoader.h/.cpp       # parseo de JSON -> LensProfile::Profile
│   ├── BundleResourcePath.h/.cpp  # ubica Contents/Resources/ del bundle en runtime
│   ├── MetalKernel.h              # firma de la funcion puente hacia Metal
│   ├── ChromaticAberrationPlugin.h/.cpp      # plugin OFX (describe/render/factory)
│   └── ChromaticAberrationProcessor.h/.cpp   # CPU fallback + llamada a Metal
├── metal/
│   └── MetalKernel.mm       # kernel Metal (Objective-C++)
└── tests/
    └── test_profile_loader.cpp  # smoke test sin OFX/Metal, corre en cualquier SO
```

## Siguiente paso

Una vez que esto compile y lo veas funcionando en Resolve, seguimos con la
Fase 2 (viñeteo + tinte de color + dropdown de selección de perfil) sobre
esta misma base de código.
