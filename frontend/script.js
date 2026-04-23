const API_URL = 'http://localhost:18080';

const form = document.getElementById('patientForm');
const tbody = document.querySelector('#patientsTable tbody');
const countEl = document.getElementById('count');
const searchResult = document.getElementById('searchResult');

function showMessage(message, isError = false) {
    searchResult.textContent = message;
    searchResult.style.color = isError ? '#b00020' : '#1b5e20';
}

function renderPatients(patients) {
    tbody.innerHTML = '';

    patients.forEach((patient) => {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${patient.id}</td>
            <td>${patient.name}</td>
            <td>${patient.age}</td>
            <td>${patient.disease}</td>
        `;
        tbody.appendChild(row);
    });

    countEl.textContent = patients.length;
}

async function fetchJson(url, options = {}) {
    const response = await fetch(url, options);
    const rawText = await response.text();
    let text = rawText.replace(/^\u0000+/, '').trim();

    if (text.startsWith('"')) {
        text = `{${text}`;
    }

    const data = JSON.parse(text);

    if (!response.ok) {
        throw new Error(data.message || 'Request failed.');
    }

    return data;
}

async function displayPatients() {
    try {
        const data = await fetchJson(`${API_URL}/patients`);
        renderPatients(data.patients || []);
    } catch (error) {
        showMessage(error.message, true);
    }
}

form.addEventListener('submit', async (event) => {
    event.preventDefault();

    const id = document.getElementById('id').value.trim();
    const name = document.getElementById('name').value.trim();
    const age = Number(document.getElementById('age').value);
    const disease = document.getElementById('disease').value.trim();

    try {
        await fetchJson(`${API_URL}/addPatient`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, name, age, disease })
        });

        form.reset();
        showMessage('Patient added successfully.');
        await displayPatients();
        document.getElementById('id').focus();
    } catch (error) {
        showMessage(error.message, true);
    }
});

document.getElementById('serveBtn').addEventListener('click', async () => {
    try {
        const data = await fetchJson(`${API_URL}/servePatient`, { method: 'DELETE' });
        if (data.served && data.patient) {
            showMessage(`Served ${data.patient.name}.`);
        } else {
            showMessage(data.message || 'No patient served.', true);
        }
        await displayPatients();
    } catch (error) {
        showMessage(error.message, true);
    }
});

document.getElementById('searchBtn').addEventListener('click', async () => {
    const searchId = document.getElementById('searchId').value.trim();
    if (!searchId) {
        showMessage('Enter a patient ID to search.', true);
        return;
    }

    try {
        const data = await fetchJson(`${API_URL}/search?id=${encodeURIComponent(searchId)}`);
        if (data.found) {
            showMessage(`Found ${data.patient.name}, Age ${data.patient.age}, Disease ${data.patient.disease}.`);
        } else {
            showMessage('Patient not found.', true);
        }
    } catch (error) {
        showMessage(error.message, true);
    }
});

document.getElementById('displayBtn').addEventListener('click', displayPatients);

window.addEventListener('load', displayPatients);
