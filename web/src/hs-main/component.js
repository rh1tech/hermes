/* eslint-disable camelcase */
export default class {
    onCreate() {
        this.state = {
            ready: false,
            error: false,
            status: {},
            settings: {},
            settingsDialogError: false,
            loading: false,
            confirmationData: false,
        };
        this.bauds = [9600, 300, 1200, 2400, 4800, 19200, 38400, 57600, 115200];
        this.baseURL = "/";
    }

    fetchWithTimeout(url, options = {}, timeout = 10000) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), timeout);
        return fetch(url, { ...options, signal: controller.signal })
            .finally(() => clearTimeout(timer));
    }

    async loadStatusAndSettings() {
        const responseStatus = await this.fetchWithTimeout(`${this.baseURL}api/get/status`);
        if (!responseStatus.ok) {
            throw new Error(`HTTP error, status: ${responseStatus.status}`);
        }
        const status = await responseStatus.json();
        this.setState("status", status);

        const responseSettings = await this.fetchWithTimeout(`${this.baseURL}api/get/settings`);
        if (!responseSettings.ok) {
            throw new Error(`HTTP error, status: ${responseSettings.status}`);
        }
        const settings = await responseSettings.json();
        this.setState("settings", settings);
    }

    showNotification(message, cls = "alert-success") {
        if (!this._notifCounter) this._notifCounter = 0;
        // eslint-disable-next-line no-plusplus
        const id = "notification-" + (++this._notifCounter);
        const el = document.createElement("div");
        el.id = id;
        el.className = `alert notification ${cls}`;
        el.textContent = message;
        // Inline transition so top changes (from _reflowNotifications) animate
        el.style.transition = "top 180ms ease, opacity 180ms ease";
        el.style.opacity = "0";
        document.body.appendChild(el);
        // Enter (fade in + initial stack)
        requestAnimationFrame(() => {
            this._reflowNotifications();
            el.style.opacity = "1";
        });
        const lifetime = 2000;
        setTimeout(() => {
            // Start fade out
            el.style.opacity = "0";
            const onEnd = (evt) => {
                if (evt.propertyName !== "opacity") return;
                el.removeEventListener("transitionend", onEnd);
                if (el.parentNode) {
                    el.parentNode.removeChild(el);
                    // After removal, reflow remaining so they animate upward
                    requestAnimationFrame(() => this._reflowNotifications());
                }
            };
            el.addEventListener("transitionend", onEnd);
        }, lifetime);
    }

    _reflowNotifications() {
        const GAP = 8; // px between notifications
        const nodes = Array.from(document.querySelectorAll(".alert.notification"));
        if (!nodes.length) return;
        // Cache the initial base top (from CSS) once so removed items don't shift the baseline
        if (this._notifBaseTop == null) {
            const firstComputed = getComputedStyle(nodes[0]);
            this._notifBaseTop = parseFloat(firstComputed.top) || 16; // fallback 16px (~1rem)
        }
        let currentTop = this._notifBaseTop;
        nodes.forEach(n => {
            n.style.top = currentTop + "px";
            currentTop += n.offsetHeight + GAP;
        });
    }

    async onMount() {
        try {
            await this.loadStatusAndSettings();
        } catch (error) {
            const message = error.name === "AbortError"
                ? "Request timed out (10s)"
                : `Could not retrieve data: ${error.message}`;
            this.setState("error", message);
        }
        this.setState("ready", true);
    }

    async onCommandButtonClick(e) {
        if (!e.target.closest("[data-id]")) {
            return;
        }
        e.preventDefault();
        const { id } = e.target.closest("[data-id]").dataset;
        switch (id) {
            case "settings":
                const dialog = document.getElementById("settingsDialog");
                if (dialog && !dialog.hasAttribute("open")) {
                    dialog.setAttribute("open", "");
                }
                const {
                    ssid = "",
                    password = "",
                    serialSpeed = "",
                    tcpServerPort = "",
                    busyMsg = ""
                } = this.state.settings || {};
                const fieldMap = {
                    s_ssid: ssid,
                    s_password: password,
                    s_serialSpeed: serialSpeed,
                    s_tcpServerPort: tcpServerPort,
                    s_busyMsg: busyMsg
                };
                let firstField = null;
                for (const [eid, val] of Object.entries(fieldMap)) {
                    const el = document.getElementById(eid);
                    if (!el) continue;
                    el.value = val ?? "";
                    if (!firstField) firstField = el;
                }
                if (firstField) firstField.focus();
                break;
            case "reboot":
                this.setState("confirmationData", {
                    message: "Are you sure you want to reboot the ESP Modem?",
                    action: "reboot"
                });
                break;
            case "hangup":
                this.setState("loading", true);
                try {
                    const res = await this.fetchWithTimeout(`${this.baseURL}api/commands/ath`, {
                        method: "GET",
                    });
                    if (!res.ok) {
                        throw new Error(`HTTP error ${res.status}`);
                    }
                    this.showNotification("Hang up successful");
                    this.loadStatusAndSettings();
                } catch {
                    this.showNotification("Could not hang up", "alert-danger");
                } finally {
                    this.setState("loading", false);
                }
                break;
            case "update":
                this.setState("confirmationData", {
                    message: "Are you sure you want to update your ESP firmware?",
                    action: "update"
                });
                break;
            case "factory":
                this.setState("confirmationData", {
                    message: "Are you sure you want to reset your ESP to factory defaults? All settings will be lost.",
                    action: "factory"
                });
                break;   
            case "eepsave":
                this.setState("confirmationData", {
                    message: "Are you sure you want to save your current settings to EEPROM?",
                    action: "eepsave"
                });
                break;
            case "eepload":
                this.setState("confirmationData", {
                    message: "Are you sure you want to load settings to EEPROM? Your current settings will be lost.",
                    action: "eepload"
                });
                break;    
        }
    }

    onSettingsDialogCancel() {
        const dialog = document.getElementById("settingsDialog");
        if (dialog && dialog.hasAttribute("open")) {
            dialog.removeAttribute("open");
        }
    }

    onSettingsDialogSave(e) {
        e.preventDefault();
        const dialog = document.getElementById("settingsDialog");
        if (dialog) {
            dialog.querySelectorAll("input, select").forEach(el => el.setAttribute("aria-invalid", "false"));
        }
        const $ = id => document.getElementById(id);
        const values = {
            ssid: $("s_ssid").value.trim(),
            password: $("s_password").value,
            serialSpeed: parseInt($("s_serialSpeed").value, 10),
            tcpServerPort: Number($("s_tcpServerPort").value),
            busyMsg: $("s_busyMsg").value.trim()
        };
        if (!values.ssid) { $("s_ssid").setAttribute("aria-invalid", "true"); return; }
        if (!values.password) { $("s_password").setAttribute("aria-invalid", "true"); return; }
        if (
            !Number.isInteger(values.tcpServerPort) ||
            values.tcpServerPort < 1 ||
            values.tcpServerPort > 65535
        ) {
            $("s_tcpServerPort").setAttribute("aria-invalid", "true");
            return;
        }
        if (!values.busyMsg) { $("s_busyMsg").setAttribute("aria-invalid", "true"); return; }
        // const { ssid, password, serialSpeed, tcpServerPort, busyMsg } = values;
        // Use form data to avoid triggering a CORS preflight (no non-simple headers)
        const formParams = new URLSearchParams();
        Object.entries(values).forEach(([k, v]) => formParams.append(k, v));
        this.setState("loading", true);
        this.setState("settingsDialogError", false);
        (async () => {
            try {
                const res = await this.fetchWithTimeout(`${this.baseURL}api/save/settings`, {
                    method: "POST",
                    mode: "cors",
                    headers: {
                        "Accept": "application/json"
                    },
                    body: JSON.stringify(values)
                });
                if (!res.ok) {
                    throw new Error(`HTTP error ${res.status}`);
                }
                await this.loadStatusAndSettings();
                this.onSettingsDialogCancel();
                this.showNotification("Settings updated");
            } catch (err) {
                this.setState("settingsDialogError", err.message || "Unknown error");
            } finally {
                this.setState("loading", false);
            }
        })();
    }

    onConfirmationDialogCancel() {
        this.setState("confirmationData", false);
    }

    async onConfirmationDialogConfirm(e) {
        if (!e.target.closest("[data-id]")) {
            return;
        }
        e.preventDefault();
        this.onConfirmationDialogCancel()
        const { id } = e.target.closest("[data-id]").dataset;
        switch (id) {
            case "reboot":
                this.setState("loading", true);
                try {
                    const res = await this.fetchWithTimeout(`${this.baseURL}api/commands/reboot`, {
                        method: "GET",
                    });
                    if (!res.ok) {
                        throw new Error(`HTTP error ${res.status}`);
                    }
                    this.showNotification("Device is rebooting");
                } catch {
                    this.showNotification("Could not restart", "alert-danger");
                } finally {
                    this.setState("loading", false);
                }
                break;
            case "update":
                this.setState("loading", true);
                try {
                    const res = await this.fetchWithTimeout(`${this.baseURL}api/commands/update`, {
                        method: "GET",
                    });
                    if (!res.ok) {
                        throw new Error(`HTTP error ${res.status}`);
                    }
                    this.showNotification("Device is updating");
                } catch {
                    this.showNotification("Could not update", "alert-danger");
                } finally {
                    this.setState("loading", false);
                }
                break;
            case "eepsave":
                this.setState("loading", true);
                try {
                    const res = await this.fetchWithTimeout(`${this.baseURL}api/commands/eeprom/save`, {
                        method: "GET",
                    });
                    if (!res.ok) {
                        throw new Error(`HTTP error ${res.status}`);
                    }
                    this.showNotification("Settings are saved");
                } catch {
                    this.showNotification("Could not save settings", "alert-danger");
                } finally {
                    this.setState("loading", false);
                }
                break;
            case "eepload":
                this.setState("loading", true);
                try {
                    const res = await this.fetchWithTimeout(`${this.baseURL}api/commands/eeprom/load`, {
                        method: "GET",
                    });
                    if (!res.ok) {
                        throw new Error(`HTTP error ${res.status}`);
                    }
                    this.showNotification("Settings are loaded");
                } catch {
                    this.showNotification("Could not load settings", "alert-danger");
                } finally {
                    this.setState("loading", false);
                }
                break;        
        }
    }
}
