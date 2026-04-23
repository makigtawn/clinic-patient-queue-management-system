#include "crow_all.h"
#include <deque>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace {

const string kDataFile = "patients.json";

struct PatientRecord {
    string id;
    string name;
    string disease;
    int age;
};

string escapeJson(const string& value) {
    string escaped;
    escaped.reserve(value.size());

    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }

    return escaped;
}

void addCorsHeaders(crow::response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(crow::response& res, const string& body, int code = 200) {
    res.code = code;
    res.body.clear();
    res.set_header("Content-Type", "application/json");
    addCorsHeaders(res);
    res.end(body);
}

void sendOptions(crow::response& res) {
    res.code = 204;
    addCorsHeaders(res);
    res.end();
}

class PatientQueue {
    deque<PatientRecord> patients_;
    string dataFile_;
    mutable mutex mtx_;

    void saveUnlocked() const {
        ofstream out(dataFile_, ios::trunc);
        out << "{\"patients\":[";

        for (size_t i = 0; i < patients_.size(); ++i) {
            const auto& patient = patients_[i];
            if (i > 0) {
                out << ",";
            }
            out << "{\"id\":\"" << escapeJson(patient.id)
                << "\",\"name\":\"" << escapeJson(patient.name)
                << "\",\"age\":" << patient.age
                << ",\"disease\":\"" << escapeJson(patient.disease) << "\"}";
        }

        out << "]}";
    }

public:
    explicit PatientQueue(const string& dataFile) : dataFile_(dataFile) {
        ifstream in(dataFile_);
        if (!in.good()) {
            saveUnlocked();
            return;
        }

        ostringstream buffer;
        buffer << in.rdbuf();
        const string content = buffer.str();
        if (content.empty()) {
            saveUnlocked();
            return;
        }

        auto json = crow::json::load(content);
        if (!json || !json.has("patients")) {
            saveUnlocked();
            return;
        }

        try {
            for (const auto& item : json["patients"]) {
                if (!item.has("id") || !item.has("name") || !item.has("age") || !item.has("disease")) {
                    continue;
                }

                patients_.push_back(PatientRecord{
                    string(item["id"].s()),
                    string(item["name"].s()),
                    string(item["disease"].s()),
                    static_cast<int>(item["age"].i())
                });
            }
        } catch (...) {
            patients_.clear();
            saveUnlocked();
        }
    }

    bool enqueue(const string& id, const string& name, int age, const string& disease) {
        lock_guard<mutex> lock(mtx_);

        for (const auto& patient : patients_) {
            if (patient.id == id) {
                return false;
            }
        }

        patients_.push_back(PatientRecord{id, name, disease, age});
        saveUnlocked();
        return true;
    }

    bool dequeue(PatientRecord* servedPatient = nullptr) {
        lock_guard<mutex> lock(mtx_);
        if (patients_.empty()) {
            return false;
        }

        if (servedPatient) {
            *servedPatient = patients_.front();
        }

        patients_.pop_front();
        saveUnlocked();
        return true;
    }

    bool search(const string& id, PatientRecord& result) const {
        lock_guard<mutex> lock(mtx_);
        for (const auto& patient : patients_) {
            if (patient.id == id) {
                result = patient;
                return true;
            }
        }
        return false;
    }

    vector<PatientRecord> traverse() const {
        lock_guard<mutex> lock(mtx_);
        return vector<PatientRecord>(patients_.begin(), patients_.end());
    }

    int getCount() const {
        lock_guard<mutex> lock(mtx_);
        return static_cast<int>(patients_.size());
    }
};

string patientsToJson(const vector<PatientRecord>& patients) {
    ostringstream out;
    out << "{\"patients\":[";

    for (size_t i = 0; i < patients.size(); ++i) {
        const auto& patient = patients[i];
        if (i > 0) {
            out << ",";
        }
        out << "{\"id\":\"" << escapeJson(patient.id)
            << "\",\"name\":\"" << escapeJson(patient.name)
            << "\",\"age\":" << patient.age
            << ",\"disease\":\"" << escapeJson(patient.disease) << "\"}";
    }

    out << "]}";
    return out.str();
}

string patientToJson(const PatientRecord& patient) {
    ostringstream out;
    out << "{\"id\":\"" << escapeJson(patient.id)
        << "\",\"name\":\"" << escapeJson(patient.name)
        << "\",\"age\":" << patient.age
        << ",\"disease\":\"" << escapeJson(patient.disease) << "\"}";
    return out.str();
}

}  // namespace

int main() {
    crow::SimpleApp app;
    PatientQueue queue(kDataFile);

    CROW_ROUTE(app, "/addPatient").methods("POST"_method, "OPTIONS"_method)(
        [&queue](const crow::request& req, crow::response& res) {
            if (req.method == crow::HTTPMethod::OPTIONS) {
                sendOptions(res);
                return;
            }

            auto body = crow::json::load(req.body);
            if (!body || !body.has("id") || !body.has("name") || !body.has("age") || !body.has("disease")) {
                sendJson(res, "{\"status\":\"error\",\"message\":\"Invalid patient data.\"}", 400);
                return;
            }

            const string id = string(body["id"].s());
            const string name = string(body["name"].s());
            const string disease = string(body["disease"].s());
            const int age = static_cast<int>(body["age"].i());

            if (id.empty() || name.empty() || disease.empty() || age <= 0) {
                sendJson(res, "{\"status\":\"error\",\"message\":\"All fields are required.\"}", 400);
                return;
            }

            if (!queue.enqueue(id, name, age, disease)) {
                sendJson(res, "{\"status\":\"error\",\"message\":\"Patient ID already exists.\"}", 409);
                return;
            }

            PatientRecord patient{id, name, disease, age};
            sendJson(res, "{\"status\":\"ok\",\"patient\":" + patientToJson(patient) + "}");
        }
    );

    CROW_ROUTE(app, "/servePatient").methods("DELETE"_method, "OPTIONS"_method)(
        [&queue](const crow::request& req, crow::response& res) {
            if (req.method == crow::HTTPMethod::OPTIONS) {
                sendOptions(res);
                return;
            }

            PatientRecord servedPatient;
            if (!queue.dequeue(&servedPatient)) {
                sendJson(res, "{\"served\":false,\"message\":\"No patients in queue.\"}");
                return;
            }

            sendJson(res, "{\"served\":true,\"patient\":" + patientToJson(servedPatient) + "}");
        }
    );

    CROW_ROUTE(app, "/patients").methods("GET"_method, "OPTIONS"_method)(
        [&queue](const crow::request& req, crow::response& res) {
            if (req.method == crow::HTTPMethod::OPTIONS) {
                sendOptions(res);
                return;
            }

            sendJson(res, patientsToJson(queue.traverse()));
        }
    );

    CROW_ROUTE(app, "/search").methods("GET"_method, "OPTIONS"_method)(
        [&queue](const crow::request& req, crow::response& res) {
            if (req.method == crow::HTTPMethod::OPTIONS) {
                sendOptions(res);
                return;
            }

            auto id = req.url_params.get("id");
            if (!id || string(id).empty()) {
                sendJson(res, "{\"found\":false,\"message\":\"Patient ID is required.\"}", 400);
                return;
            }

            PatientRecord patient;
            if (!queue.search(id, patient)) {
                sendJson(res, "{\"found\":false}");
                return;
            }

            sendJson(res, "{\"found\":true,\"patient\":" + patientToJson(patient) + "}");
        }
    );

    CROW_ROUTE(app, "/count").methods("GET"_method, "OPTIONS"_method)(
        [&queue](const crow::request& req, crow::response& res) {
            if (req.method == crow::HTTPMethod::OPTIONS) {
                sendOptions(res);
                return;
            }

            sendJson(res, "{\"count\":" + to_string(queue.getCount()) + "}");
        }
    );

    app.port(18080).run();
}
