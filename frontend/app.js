const API = "https://banking-queue-backend.onrender.com/api";
const $ = (selector) => document.querySelector(selector);
const state = { queue: null, history: [] };

function setConnectionStatus(online, message) {
  const dot = $("#connectionDot");
  const text = $("#connectionText");
  const container = dot.parentElement;
  if (online) {
    container.classList.remove("offline");
    container.classList.add("online");
    text.textContent = message || "C++ server online";
  } else {
    container.classList.remove("online");
    container.classList.add("offline");
    text.textContent = message || "Unable to connect to C++ queue server.";
  }
}

async function request(path, options = {}) {
  try {
    const response = await fetch(`${API}${path}`, {
      headers: { "Content-Type": "application/json" },
      ...options
    });
    const data = await response.json();
    if (!response.ok) {
      throw new Error(data.message || "The request could not be completed.");
    }
    // Any successful HTTP response means server is online
    setConnectionStatus(true, "C++ server online");
    return data;
  } catch (error) {
    if (error instanceof TypeError || error.name === "TypeError" || (error.message && error.message.includes("fetch"))) {
      setConnectionStatus(false, "Unable to connect to C++ queue server.");
      throw new Error("Unable to connect to C++ queue server.");
    }
    throw error;
  }
}

function setMessage(element, text, type = "success") {
  element.textContent = text;
  element.className = `form-message ${type}`;
}

function escapeHtml(value) {
  if (!value) return "";
  return String(value).replace(/[&<>"']/g, (character) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#039;"
  }[character]));
}

function customerRow(customer) {
  const priority = customer.type === "VIP" && customer.priorityLabel
    ? `<span class="priority ${customer.priorityLabel.toLowerCase()}">${customer.priorityLabel}</span>`
    : "";
  return `<div class="queue-item">
    <span class="token">${escapeHtml(customer.token)}</span>
    <span class="queue-info">
      <b>${escapeHtml(customer.name)}</b>
      <small>${customer.type}</small>
    </span>
    ${priority}
  </div>`;
}

function render() {
  const { queue } = state;
  if (!queue) return;

  $("#normalWaiting").textContent = String(queue.normalWaiting).padStart(2, "0");
  $("#vipWaiting").textContent = String(queue.vipWaiting).padStart(2, "0");
  $("#totalWaiting").textContent = String(queue.totalWaiting).padStart(2, "0");
  $("#totalServed").textContent = String(queue.totalServed).padStart(2, "0");

  const next = queue.nextCustomer;
  if (next) {
    $("#nextToken").textContent = next.token;
    $("#nextName").textContent = next.name;
    $("#nextDesk").textContent = next.assignedDesk || "—";
    if (next.type === "VIP") {
      $("#nextMeta").textContent = `VIP · ${next.priorityLabel || ""}`;
      $("#nextMeta").className = "badge vip";
    } else {
      $("#nextMeta").textContent = "Normal · FIFO";
      $("#nextMeta").className = "badge";
    }
  } else {
    $("#nextToken").textContent = "—";
    $("#nextName").textContent = "No customers waiting";
    $("#nextDesk").textContent = "—";
    $("#nextMeta").textContent = "Queue empty";
    $("#nextMeta").className = "badge";
  }
  $("#serveButton").disabled = !next || isServing;

  renderDesks(queue);
}

function renderDesks(queue) {
  const desks = queue.desks || [];
  $("#deskCards").innerHTML = desks.map((desk) => {
    const next = desk.nextCustomer;
    const nextLine = next
      ? `<b>${escapeHtml(next.token)}</b> <span>${escapeHtml(next.name)}</span>${next.type === "VIP" ? ` <em>${escapeHtml(next.priorityLabel)}</em>` : ""}`
      : '<span class="desk-empty">No customers waiting</span>';
    const width = desk.totalWaiting ? Math.min(100, desk.totalWaiting * 16) : 0;
    return `<article class="desk-card">
      <div class="desk-card-heading"><h3>${escapeHtml(desk.name)}</h3><span>${desk.totalWaiting} waiting</span></div>
      <div class="load-bar"><span style="width:${width}%"></span></div>
      <div class="desk-stats"><span>Normal <b>${desk.normalWaiting}</b></span><span>VIP <b>${desk.vipWaiting}</b></span><span>Served <b>${desk.servedCount}</b></span></div>
      <div class="desk-next"><small>NEXT CUSTOMER</small><div>${nextLine}</div></div>
      <button class="button button-primary full desk-serve" data-desk-id="${desk.id}" ${next ? "" : "disabled"}>Serve next <span>→</span></button>
    </article>`;
  }).join("") || '<div class="empty">No service counters available</div>';

  document.querySelectorAll(".desk-serve").forEach((button) => {
    button.addEventListener("click", () => serveDesk(Number(button.dataset.deskId), button));
  });
}

function renderHistory() {
  $("#history").innerHTML = state.history && state.history.length
    ? state.history.map((customer) => {
        const timeStr = customer.servedTime ? ` · ${escapeHtml(customer.servedTime)}` : "";
        return `<div class="history-item">
          <span class="token">${escapeHtml(customer.token)}</span>
          <b>${escapeHtml(customer.name)}</b>
          <small>${customer.type} · ${escapeHtml(customer.assignedDesk || "Desk —")}${customer.priorityLabel ? ` · ${escapeHtml(customer.priorityLabel)}` : ""}${timeStr}</small>
          <span class="served-badge">SERVED</span>
        </div>`;
      }).join("")
    : '<div class="empty">No completed services yet</div>';
}

let refreshInFlight = null;
async function refresh() {
  if (refreshInFlight) return refreshInFlight;

  refreshInFlight = (async () => {
    const [queue, history] = await Promise.all([
      request("/queue"),
      request("/history")
    ]);
    state.queue = queue;
    state.history = history.history;
    render();
    renderHistory();
    setConnectionStatus(true, "C++ server online");
  })();

  try {
    await refreshInFlight;
  } finally {
    refreshInFlight = null;
  }
}

// Form toggles
$("input[name='customerType'][value='NORMAL']").addEventListener("change", () => {
  $("#priorityWrap").classList.add("hidden");
});
$("input[name='customerType'][value='VIP']").addEventListener("change", () => {
  $("#priorityWrap").classList.remove("hidden");
});

// Intake Customer
$("#customerForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const nameInput = $("#customerName");
  const name = nameInput.value.trim();
  if (!name) {
    setMessage($("#formMessage"), "Please enter customer name.", "error");
    nameInput.focus();
    return;
  }

  const typeRadio = document.querySelector("input[name='customerType']:checked");
  const type = typeRadio ? typeRadio.value : "NORMAL";
  const priorityVal = Number($("#priority").value);
  if (type === "VIP" && (!priorityVal || priorityVal < 1 || priorityVal > 3)) {
    setMessage($("#formMessage"), "Please select a valid VIP priority (1=High, 2=Medium, 3=Low).", "error");
    return;
  }

  const submitButton = event.target.querySelector("button[type='submit']");
  submitButton.disabled = true;

  try {
    const data = await request("/customer", {
      method: "POST",
      body: JSON.stringify({ name, type, priority: priorityVal })
    });
    const assigned = data.assignedDesk || data.customer?.assignedDesk || "the selected counter";
    const queueType = type === "VIP"
      ? `VIP · ${data.customer?.priorityLabel || $("#priority option:checked").textContent}`
      : "Normal · FIFO";
    setMessage($("#formMessage"), `Customer added successfully. Token: ${data.token} · Assigned: ${assigned} · ${queueType}`, "success");
    event.target.reset();
    $("#priorityWrap").classList.add("hidden");
    const normalRadio = document.querySelector("input[name='customerType'][value='NORMAL']");
    if (normalRadio) normalRadio.checked = true;
    await refresh();
  } catch (error) {
    setMessage($("#formMessage"), error.message, "error");
  } finally {
    submitButton.disabled = false;
  }
});

// Serve Next Customer
let isServing = false;
async function serveDesk(deskId, sourceButton = $("#serveButton")) {
  if (isServing) return;
  isServing = true;
  const button = sourceButton;
  button.disabled = true;

  try {
    const body = deskId > 0 ? JSON.stringify({ deskId }) : undefined;
    const data = await request("/serve", { method: "POST", body });
    const cust = data.customer;
    const timeStr = cust.servedTime ? ` at ${cust.servedTime}` : "";
    const priorityStr = cust.priorityLabel ? ` · ${cust.priorityLabel}` : "";
    setMessage(
      $("#serveMessage"),
      `Now serving ${cust.token} — ${cust.name} from ${cust.assignedDesk || data.servedDesk || "desk"} (${cust.type}${priorityStr})${timeStr}`,
      "success"
    );
    await refresh();
  } catch (error) {
    setMessage($("#serveMessage"), error.message, "error");
  } finally {
    isServing = false;
    button.disabled = false;
  }
}

$("#serveButton").addEventListener("click", () => serveDesk(0));

// Reset Simulator
let isResetting = false;
$("#resetButton").addEventListener("click", async () => {
  if (isResetting) return;
  if (!confirm("Are you sure you want to reset the simulator? This will clear all waiting queues, history, and counters.")) {
    return;
  }
  isResetting = true;
  const button = $("#resetButton");
  button.disabled = true;

  try {
    const data = await request("/reset", { method: "POST" });
    setMessage($("#serveMessage"), data.message || "Simulator reset successfully.", "success");
    setMessage($("#formMessage"), "");
    await refresh();
  } catch (error) {
    showToast(error.message);
  } finally {
    isResetting = false;
    button.disabled = false;
  }
});

// Load Demo Data
let isDemoLoading = false;
$("#demoButton").addEventListener("click", async () => {
  if (isDemoLoading) return;
  isDemoLoading = true;
  const button = $("#demoButton");
  button.disabled = true;

  try {
    const data = await request("/demo", { method: "POST" });
    showToast(data.message || "Demo customers added through C++ queues.");
    setMessage($("#formMessage"), "");
    await refresh();
  } catch (error) {
    showToast(error.message);
  } finally {
    isDemoLoading = false;
    button.disabled = false;
  }
});

function showToast(message) {
  const toast = $("#toast");
  toast.textContent = message;
  toast.classList.add("show");
  setTimeout(() => toast.classList.remove("show"), 3200);
}

// Initial connection check
refresh().catch((error) => {
  setConnectionStatus(false, "Unable to connect to C++ queue server.");
});

// Background heartbeat to reconnect automatically when C++ server starts
setInterval(() => {
  refresh().catch(() => {
    setConnectionStatus(false, "Unable to connect to C++ queue server.");
  });
}, 4000);
