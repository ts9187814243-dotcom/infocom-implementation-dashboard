	# ОТЧЁТ ПО ЛАБОРАТОРНОЙ РАБОТЕ № 1

**Дисциплина:** Инфраструктура и средства командной разработки / Внедрение ИС
**Тема:** Инициализация репозитория, организация командной работы по методологии GitFlow, контейнеризация и слияние модулей проекта
**Проект:** `infocom-implementation-dashboard`
**Состав команды:**
* **Трушинский Степан** — Team Lead / DevOps / QA (`ts9187814243-dotcom`)
* **Тагиров Эдгар** — Fullstack Developer: Backend (C++) & Frontend (React) (`OlekGargamel`)

## 1. Цель работы

Освоить практические навыки командной разработки ПО:
1. Создание и настройка публичного репозитория GitHub, доски проектов (GitHub Projects) и установление правил ветвления (GitFlow).
2. Создание шаблонов Pull Request и правил командной работы (`CONTRIBUTING.md`).
3. Разработка и контейнеризация сервисов (C++ Backend, React Frontend, Docker Compose).
4. Организация перекрёстного Code Review, проведение Pull Request и объединение индивидуальных модулей в общую ветку `develop` с закрытием задач на канбан-доске.

## 2. Распределение задач и роли в команде

В соответствии с методическими требованиями в проекте были сформированы задачи и созданы соответствующие Issues:

| Идентификатор | Название задачи | Ответственный | Результат |
| :--- | :--- | :--- | :--- |
| **`IC-1`** | Инициализация репозитория, структура и регламент `CONTRIBUTING.md` | Степан (TL/DevOps) | Закрыто (Done) |
| **`IC-4`** | Настройка Docker-окружения (`docker-compose.yml`) и шаблона PR | Степан (TL/DevOps) | Закрыто (Done) |
| **`IC-2`** | Подготовка C++ Backend с эндпоинтом `/health` и `Dockerfile` | Эдгар (Backend) | Закрыто (Done) |
| **`IC-3`** | Подготовка Frontend (React + Vite) со списком команды и `Dockerfile` | Эдгар (Frontend) | Закрыто (Done) |

## 3. Ход выполнения работы по этапам

Инициализация репозитория и регламента (Степан — TL/DevOps)
* **Задачи `IC-1` и `IC-4`**:
* В ветке `feature/IC-01_tl_qa_setup` были созданы основные служебные файлы проекта: `CONTRIBUTING.md` (регламент коммитов и веток), `.github/PULL_REQUEST_TEMPLATE.md` (шаблон описания PR), `docker-compose.yml` и базовый файл отчёта `docs/lab1_report.md`.
* Оформлен и одобрен **Pull Request #5** в ветку `develop`.
* Проведено слияние (Merge), ветка удалена, задачи `IC-1` и `IC-4` переведены в статус **Done** на доске `Панель управления внедрением ИС`.

<img width="1794" height="957" alt="PR Team lead/Devops/QA" src="https://github.com/user-attachments/assets/769503c8-bc82-4e78-b5e8-d4e18d71859e" />

<img width="1840" height="935" alt="Выполнены задачи 1 и 4" src="https://github.com/user-attachments/assets/d4d0ae9e-49b2-4c99-8f0b-d7176df5d9ea" />

Разработка и Code Review модуля C++ Backend (Эдгар — Backend)
* **Задача `IC-2`**:
* Разработчик `OlekGargamel` создал ветку `feature/IC-01_backend_initial_setup` от `develop`.
* В каталоге `backend/` разработано серверное C++ приложение с использованием библиотеки `httplib.h`, реализующее обработку запроса `GET /health` с ответом `{"status":"ok"}`.
* Подготовлен многоэтапный `backend/Dockerfile` (`gcc:13` $\rightarrow$ `scratch`).
* **Code Review**:
* В ходе первого анализа кода выявлены опечатка в объекте сервера (`dvr.Get` вместо `svr.Get`) и несовпадение портов (`8000` вместо `8080`).
* Разработчик внес корректировки (коммит `The typo has been corrected.`), после чего **Pull Request #6** был успешно одобрен (Approve) и слиты изменения в `develop`.

<img width="1787" height="956" alt="PR backend" src="https://github.com/user-attachments/assets/6db1db09-4c56-4d4f-a33b-388933406dd7" />


<img width="1818" height="936" alt="Выполнена задача 2" src="https://github.com/user-attachments/assets/4fb6d849-4e7e-41d4-aebc-91eb7a8a4681" />


Разработка и Code Review модуля Frontend (Эдгар — Frontend)
* **Задача `IC-3`**:
* В ветке `feature/IC-01_frontend_initial_setup` инициализирован React-проект на базе Vite.
* В файле `frontend/src/App.jsx` сверстан компонент с отображением названия проекта `Infocom Implementation Dashboard` и списка членов команды (`Трушинский Степан - Team Lead`, `Тагиров Эдгар - Fullstack`).
* Создан multi-stage `frontend/Dockerfile` (`node:20-alpine` для сборки $\rightarrow$ `nginx:alpine` для отдачи статики).
* Оформлен **Pull Request #7**.
* **Code Review**:
* Проверена структура файлов (`App.jsx`, `main.jsx`, `index.html`, `package.json`, `vite.config.js`).
* Выполнено согласование портов с глобальным `docker-compose.yml`, оставлен ревью-комментарий, после чего PR был подтвержден (`Confirm merge`) и влит в `develop`.

<img width="907" height="433" alt=" Создание файлов и структуры фронтенда в консоли Git Bash" src="https://github.com/user-attachments/assets/db3224b8-5d92-461a-a4f9-bbffd85b3f4e" />
<img width="1232" height="517" alt="Код файла App.jsx " src="https://github.com/user-attachments/assets/a0461715-bd5b-4b9a-934d-046e07861d60" />
<img width="513" height="251" alt="Код файла  Dockerfile" src="https://github.com/user-attachments/assets/7a70967f-e949-468b-8405-c859eefa84f4" />
<img width="1803" height="951" alt="PR frontend" src="https://github.com/user-attachments/assets/4d2c3d0f-f060-4e4e-9366-5691a67de3d5" />
<img width="1793" height="951" alt="Все задачи выполнены" src="https://github.com/user-attachments/assets/de8bfd8c-4b95-45c3-a3b8-c0cd39a18fb2" />

<img width="472" height="920" alt="Структура репозитория" src="https://github.com/user-attachments/assets/ffb07616-02d2-4dc5-8941-a6afac5e5d36" />
<img width="1920" height="1033" alt="Скриншот работы docker-compose up" src="https://github.com/user-attachments/assets/b6bde01b-6920-482d-9231-646e746e4b0e" />
<img width="1920" height="1025" alt="Скриншот работы docker-compose up" src="https://github.com/user-attachments/assets/265e196d-c075-4010-86d2-ccda241f34a7" />
<img width="1299" height="801" alt="Скриншот работающего приложения" src="https://github.com/user-attachments/assets/ad2a1595-bdd0-42c2-9b5e-512c4423bd4a" />


## 5. Вывод

В результате выполнения лабораторной работы была успешно развернута и апробирована инфраструктура командной разработки веб-проекта `infocom-implementation-dashboard`.
Были полностью настроены правила ветвления и оформления кода, создана единая система сборки на базе Docker Compose, а модули C++ Backend и React Frontend успешно прошли стадию Code Review и интегрированы в единую рабочую ветку `develop`. Работа выполнена в полном объёме согласно всем критериям методических указаний.
