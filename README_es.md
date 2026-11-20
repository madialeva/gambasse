<p align="center">
  <img src="resources/img/logo.svg" alt="Gambasse" width="128" height="128" />
</p>

<h1 align="center">Gambasse</h1>

<p align="center">
  <a href="https://github.com/madialeva/gambasse/actions/workflows/ci-linux.yml"><img src="https://github.com/madialeva/gambasse/actions/workflows/ci-linux.yml/badge.svg" alt="ci-linux" /></a>
  <a href="LICENSE.md"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License" /></a>
  <a href="#"><img src="https://img.shields.io/badge/c%2B%2B-26-00599C?logo=cplusplus&logoColor=white" alt="C++26" /></a>
  <a href="#"><img src="https://img.shields.io/badge/Qt-6.12-41CD52?logo=qt&logoColor=white" alt="Qt 6.12" /></a>
  <a href="#"><img src="https://img.shields.io/badge/CMake-3.25-064F8C?logo=cmake&logoColor=white" alt="CMake" /></a>
  <a href="#"><img src="https://img.shields.io/badge/GCC-16-003057?logo=gcc&logoColor=white" alt="GCC 16" /></a>
  <a href="#"><img src="https://img.shields.io/badge/SQLite-003B57?logo=sqlite&logoColor=white" alt="SQLite" /></a>
</p>

<p align="left">
  <a href="README.md">Inglés</a> | <strong>Español</strong>
</p>

<p align="left">
  Gambasse es una aplicación de historia clínica y consulta para ONG o cualquier
  profesional sanitario que trabaje con pacientes en zonas remotas y humildes, donde
  no existe censo de población y las personas carecen de documentos de identidad. Como
  no hay un documento oficial para localizar a cada paciente, la aplicación registra con
  quién convive para poder encontrarlo y hacerle seguimiento más adelante; además, la
  historia médica recoge el contexto social, como la convivencia con animales domésticos
  o el acceso a agua corriente tratada, así como la fecha de nacimiento cuando se
  conoce —en muchos casos no existen partidas de nacimiento y las personas no saben
  cuándo nacieron—. Está desarrollada en <strong>C++</strong> con
  <strong>Qt Quick (QML)</strong> para la interfaz y <strong>SQLite</strong> para el
  almacenamiento, una elección que permite ejecutarla en ordenadores poco potentes
  manteniendo un rendimiento excelente.
</p>

---

## Pensada para el trabajo clínico diario

Gambasse reúne en un mismo programa de escritorio, ágil y sencillo, el registro de pacientes, las historias clínicas y las hojas de consulta para médicos, enfermeras y equipos de salud comunitaria.

### Registro y búsqueda de pacientes sin documento de identidad

- **Datos que ayudan a volver a encontrar a cada persona:** registra nombre, sexo, fecha de nacimiento (cuando se conoce) o edad aproximada, dirección, **convivientes** (con quién vive), una **persona de contacto** y el número de hermanos.
- **Fotografías de pacientes:** adjunta una foto para confirmar la identidad en próximas visitas. Las fotografías hechas con el teléfono móvil se giran automáticamente a su orientación correcta.
- **Búsqueda y ordenación rápidas:** escribe en el cuadro de filtro sobre cualquier columna para acotar el listado al instante, o pulsa en la cabecera de cualquier columna para ordenar alfabéticamente, por código o por edad.
- **Protección frente a duplicados:** avisa si ya existe un paciente registrado con el mismo nombre, sexo, fecha de nacimiento y edad, evitando registros repetidos.

### Historias clínicas adaptadas a cada paciente

Gambasse ofrece automáticamente las historias clínicas adecuadas según la edad y el sexo de cada paciente:

- **Historia pediátrica** *(hasta los 16 años)*: alergias, peso y talla al nacer, incidencias en el parto, antecedentes del niño, condiciones de la vivienda e higiene, alimentación, tratamientos previos y registro de vacunación.
- **Historia de adulto** *(mayores de 16 años)*: alergias, enfermedades crónicas (como diabetes, hepatitis o VIH y sus tratamientos), ocupación, contexto familiar y social, condiciones de la vivienda (agua potable, uso de mosquitera, letrina, animales domésticos) y constantes vitales iniciales.
- **Historia de embarazo** *(mujeres a partir de los 9 años)*: alergias, antecedentes obstétricos y médicos, condiciones de la vivienda, exploración física (altura uterina, análisis de orina, edemas) y suplementos o tratamientos anteriores y actuales.

### Consultas y seguimiento de visitas

Una vez creada la historia clínica de un paciente, se pueden registrar consultas de **Pediatría**, **Adulto** o **Embarazo** y revisar todas las visitas anteriores en orden cronológico desde la misma ventana:

- Según el tipo de consulta, permite documentar el motivo de la consulta, el diagnóstico de enfermería, la exploración física por aparatos, los tratamientos pautados y las recomendaciones de cuidado infantil.
- En las consultas de adulto, cuenta con un diagrama corporal numerado para anotar la región y descripción de las lesiones.

### Cómoda para todo el equipo

- **Tres idiomas incluidos:** cambia en directo desde la barra superior entre **portugués** (norma de Guinea-Bisáu / portugués europeo), **español** e **inglés**. Tanto el calendario (semanas empezando en lunes, fechas `dd/MM/yyyy`) como la ordenación de la lista de pacientes se adaptan automáticamente al idioma elegido.
- **Temas claro y oscuro:** alterna entre el modo claro para consultas bien iluminadas y el modo oscuro para entornos con poca luz.
- **Zoom ajustable en todas las pantallas:** amplía tanto la pantalla principal de pacientes como las ventanas de historia y consulta hasta el doble de su tamaño con los botones de lupa de la barra de título —o con `Ctrl` + `+`, `Ctrl` + `-`, y `Ctrl` + `0` para restablecer— para leer con más comodidad sin salirse nunca de la pantalla.
- **Tipografía clara y disposición recordada:** incluye su propia fuente para que todos los textos se vean nítidos en cualquier ordenador, arranca maximizada por defecto en pantallas grandes y recuerda automáticamente el tamaño de ventana, el idioma, el tema y el tamaño de lectura entre sesiones.

## Sin conexión, portátil y fácil de respaldar

- **Sin internet ni servidor:** todos los historiales permanecen en el propio ordenador, por lo que funciona con fiabilidad en lugares sin conectividad.
- **Sin instalación:** se ejecuta directamente en **Windows** (Windows 10 o posterior) y **Linux de 64 bits** sin necesitar permisos de administrador.
- **Copias de seguridad sencillas:** los datos de los pacientes, historias clínicas, consultas y fotografías se guardan juntos en la misma carpeta del programa (`database.db` y la carpeta `fotos/`). Para hacer una copia de seguridad o trasladar los datos a otro ordenador, basta con copiar toda la carpeta.
- **Actualizaciones seguras:** si ya dispones de datos de una versión anterior de Gambasse, los registros y fotografías existentes se conservan y actualizan automáticamente la primera vez que se abre la nueva versión.
- **Versión y avisos:** abre **Ayuda → Información** en la barra superior en cualquier momento para ver la versión instalada y los avisos de licencia.

## Para desarrolladores

¿Te interesa compilar Gambasse desde el código fuente, contribuir o empaquetarlo para Linux y Windows? La documentación técnica —ramas y versionado, configuración y migraciones de base de datos, requisitos del entorno, compilación en Linux/Windows/estática, flujo de QML, diagnósticos e internacionalización— está disponible (en inglés) en [Documentación para desarrolladores](docs/README-DEVELOPER.md).

## Licencia

Publicado bajo la **Licencia MIT**. Eres libre de usar, modificar y distribuir este
software, incluso en proyectos privativos, siempre que conserves el aviso de
copyright original. Consulta [LICENSE.md](LICENSE.md) para más detalles.

Qt se utiliza bajo la licencia LGPL v3. Los ejecutables autocontenidos de Linux y
Windows la enlazan de forma estática: quien los reciba debe poder volver a
enlazarlos con otra versión de Qt, algo que permiten las fuentes públicas de este
proyecto y sus scripts de compilación. Todo ejecutable autocontenido que se
distribuya debe ir acompañado del texto de la LGPL y de los avisos de Qt.

La tipografía de la interfaz, Noto Sans (Regular y Bold, sin modificar), está
incrustada en los ejecutables bajo la licencia SIL Open Font License 1.1, a la que
no sustituye la Licencia MIT. Sus avisos y licencia se encuentran en
`resources/fonts/OFL.txt`, y también pueden consultarse desde el diálogo de
Información del menú Ayuda.
