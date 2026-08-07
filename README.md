# Dungeon crawler

Многопользовательская игра в жанре Dungeon Crawler с клиент-серверной архитектурой.

### Предварительные требования

> ⚠️ **Примечание для пользователей Linux:**  
> Проект использует зависимость **X11** (файл: `src/common/keyboard_reader.h`).  
> Данная зависимость отсутствует в **conanfile.py** (иначе требуется вызов conan через sudo).  
> Подключается локально через **find_package()** в файле `src/common/CMakeLists.txt`.  
> Во избежание ошибок сборки, установите пакет разработки **X11** вручную через системный пакетный менеджер перед
> запуском
> Conan.
>
> **Ubuntu / Debian:**
> ```bash
> sudo apt update && sudo apt install -y libx11-dev
> ```

## Сборка

### 1. Подготовка окружения (Conan)

Установите [**Conan 2.x**](https://github.com/conan-io/conan) и создайте профиль по умолчанию:

```bash
conan profile detect
```

Чтобы проверить настройки компилятора, откройте созданный файл профиля. Найти его расположение на любой ОС можно
командой:

```bash
conan profile path default
```

Убедитесь, что в файле выставлена строчка: `compiler.cppstd=20`.
*Примечание для Linux: ваш GCC должен быть версии 11+, а Clang — 13+, чтобы поддерживать C++20.*

### 2. Установка зависимостей

Выполните в корневом каталоге проекта (`Podzemeliya_First_iteration/`):

```bash
conan install . -s build_type=Debug --build=missing
```

Папка `build/` и файлы конфигурации CMake (`CMakePresets.json`) создадутся автоматически.

### 3. Компиляция проекта

#### Вариант А: Сборка в IDE (CLion / VS Code)

* **CLion:** Откройте корневой `CMakeLists.txt`. IDE автоматически обнаружит Conan-пресеты сборки.
* **VS Code:** Откройте корневую папку. Установите расширение **CMake Tools**. В нижней панели выберите пресет настройки
  `conan-default` и пресет сборки `conan-debug`.

#### Вариант Б: Сборка через консоль (Linux / Windows Terminal)

Выполните последовательно из корня проекта:

```bash
# Генерация файлов сборки 
cmake --preset conan-default
# или
cmake --preset conan-debug
# или
cmake --preset conan-release

# Сборка сервера
cmake --build --preset conan-debug --target dungeons_server

# Сборка клиента
cmake --build --preset conan-debug --target dungeons_client
```

💡 Если используется conanfile.py, для генерации cmake и сборки можно использовать:

```bash
conan build . -s build_type=Debug
```
