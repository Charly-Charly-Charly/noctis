# Publicar una versión

Los releases se arman solos en GitHub Actions al empujar un tag `vX.Y.Z`.

## Pasos

1. Subir `VERSION` en el `project(noctis …)` de `CMakeLists.txt` (por ejemplo `0.4.0`).
2. Opcional: escribir las notas en `release-notes/v0.4.0.md`. Si no existe, GitHub las
   genera a partir de los commits.
3. Commit y push a `main`; esperar a que el workflow **CI** quede en verde.
4. Crear y empujar el tag:

   ```bash
   git tag v0.4.0
   git push origin v0.4.0
   ```

El workflow **Release** compila con Qt + MinGW, corre los tests, arma el paquete
(`windeployqt` + diccionario), hace una prueba de humo del `.exe` empaquetado y publica
`noctis-v0.4.0-windows-x64.zip` en el release.

## Reglas

- El tag tiene que coincidir con `VERSION` de `CMakeLists.txt`; si no, el workflow falla
  antes de compilar (el aviso de actualizaciones de la app compara contra ese número).
- Un tag con sufijo (`v0.4.0-beta.1`) se publica como **prerelease**, y la app no avisa de
  los prereleases.
- Si el release falla a mitad, se puede volver a empezar: borrar el release y el tag
  (`gh release delete v0.4.0 --cleanup-tag`), corregir, y crear el tag de nuevo.

## Probar el empaquetado sin publicar

El workflow **CI** (`Actions` → `CI` → `Run workflow`) hace el mismo build, los tests y el
paquete, y deja el zip como artefacto descargable durante 14 días, sin crear ningún release.

## Dónde ajustar

- Versión de Qt: variable `QT_VERSION` en `.github/workflows/build.yml`.
- Contenido del zip: paso «Armar el paquete» del mismo archivo.
