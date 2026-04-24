# Clinic Patient Queue Management System

A lightweight full-stack application for managing a clinic’s patient queue. The backend is built in C++ using the Crow framework, while the frontend uses plain HTML, CSS, and JavaScript.

## Project Structure

```
clinic-patient-queue/
│
├── backend/
│   ├── main.cpp
│   ├── crow_all.h         # Download from Crow releases
│   └── CMakeLists.txt
│
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── script.js
```

## Getting Started

### Prerequisites

- CMake (3.0)
- C++ compiler (recommended: g++)
- Node.js (optional, for serving frontend)

## Backend Setup (C++)

1. Download `crow_all.h` from the official Crow releases:
   [https://github.com/CrowCpp/Crow/releases](https://github.com/CrowCpp/Crow/releases)

2. Place `crow_all.h` inside the `backend/` directory.

3. Build and run the server:

```
cd backend
mkdir build
cd build
cmake ..
make
./clinic_queue_server
```

4. The backend will start on:

```
http://localhost:8080
```

## Frontend Setup

### Option to Serve with Node.js

This avoids potential CORS issues.

```
cd frontend
npx http-server -p 8000
```

Then open:

```
http://localhost:8000
```

## Features

- Add new patients to the queue
- Serve (remove) patients in FIFO order
- Search for patients
- Display full queue
- Count total patients
- Backend queue implemented using a linked list in C++
- Simple and clean UI
- REST-like communication using HTTP (Fetch API)

## System Workflow

1. User interacts with the frontend UI
2. Frontend sends HTTP requests to the backend
3. Backend processes requests using queue logic (C++)
4. Backend returns JSON responses
5. Frontend updates the UI dynamically

## Notes

- This project is designed for learning purposes (data structures + system integration).
- The queue implementation is manual (linked list), not STL-based, to reinforce core concepts.
- it can extended with:
  - Persistence (file or database)
  - Authentication
  - Priority queue (e.g., emergency patients)

## Possible Improvements

- Replace linked list with priority queue for triage systems
- Add API validation and error handling
- Introduce logging for backend operations
- Convert frontend into a modern framework (React, Vue, etc.)
- Add unit tests for queue operations
