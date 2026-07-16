Despues de instalar:

  1. Abri (o reabri) DaVinci Resolve.
  2. Anda al panel Effects -> OpenFX -> grupo "Bitacora Films".
  3. Arrastra "Lens Profile - Aberracion Cromatica" sobre un clip en el timeline.
  4. El slider "Caracter" (0-150%) controla la intensidad del efecto.

Si el efecto no aparece en el panel Effects:

  - Confirma que DaVinci Resolve se reinicio despues de instalar (no alcanza
    con haber estado abierto antes -- Resolve lee los plugins OFX solo al
    arrancar).
  - Revisa que el efecto quedo en /Library/OFX/Plugins/LensProfileOFX.ofx.bundle
    (Finder -> Ir -> Ir a la carpeta... -> /Library/OFX/Plugins).

Para desinstalar, borra manualmente:
  /Library/OFX/Plugins/LensProfileOFX.ofx.bundle

Contacto: produccion@bitacorafilms.cl
