document.addEventListener('DOMContentLoaded', () => {
    let currentFilter = 'all';
    let tasks = [];

    const tasksContainer = document.getElementById('tasks-list-container');
    const emptyState = document.getElementById('empty-state');
    const form = document.getElementById('add-task-form');
    const titleInput = document.getElementById('task-title-input');
    const scopeButtons = document.querySelectorAll('.scope-btn');

    // Telemetry HUD elements
    const statTotal = document.getElementById('stat-total');
    const statPending = document.getElementById('stat-pending');
    const statDone = document.getElementById('stat-done');

    const countAll = document.getElementById('count-all');
    const countPending = document.getElementById('count-pending');
    const countDone = document.getElementById('count-done');

    // Format numbers with leading zeros (e.g. "04")
    const pad = (n) => String(n).padStart(2, '0');

    // Fetch tasks from API
    async function loadTasks() {
        try {
            const res = await fetch('/api/tasks');
            if (!res.ok) throw new Error('API connection error');
            tasks = await res.json();
            renderLedger();
            updateTelemetry();
        } catch (err) {
            console.error('Failed to load ledger:', err);
        }
    }

    // Render task items in industrial tactical format
    function renderLedger() {
        tasksContainer.innerHTML = '';

        const filtered = tasks.filter(t => {
            if (currentFilter === 'pending') return !t.done;
            if (currentFilter === 'done') return t.done;
            return true;
        });

        if (filtered.length === 0) {
            emptyState.classList.remove('hidden');
        } else {
            emptyState.classList.add('hidden');
        }

        filtered.forEach(task => {
            const prioUpper = (task.priority || 'med').toUpperCase();
            const prioClass = (task.priority || 'med').toLowerCase();
            const card = document.createElement('article');
            card.className = `directive-card ${task.done ? 'resolved' : ''}`;
            card.dataset.priority = prioClass;

            card.innerHTML = `
                <button class="glyph-toggle" data-id="${task.id}" aria-label="${task.done ? 'Mark pending' : 'Mark resolved'}">
                    <svg class="glyph-check" viewBox="0 0 24 24">
                        <polyline points="20 6 9 17 4 12"></polyline>
                    </svg>
                </button>
                <div class="directive-index">#${pad(task.id)}</div>
                <div class="directive-body">
                    <span class="directive-title">${escapeHtml(task.title)}</span>
                </div>
                <div class="prio-badge ${prioClass}">
                    ${prioUpper}
                </div>
                <button class="purge-btn" data-id="${task.id}" title="Purge directive">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <line x1="18" y1="6" x2="6" y2="18"></line>
                        <line x1="6" y1="6" x2="18" y2="18"></line>
                    </svg>
                </button>
            `;

            tasksContainer.appendChild(card);
        });

        // Event hooks
        document.querySelectorAll('.glyph-toggle').forEach(btn => {
            btn.addEventListener('click', () => toggleTask(parseInt(btn.dataset.id, 10)));
        });

        document.querySelectorAll('.purge-btn').forEach(btn => {
            btn.addEventListener('click', () => deleteTask(parseInt(btn.dataset.id, 10)));
        });
    }

    // Update Telemetry HUD metrics
    function updateTelemetry() {
        const total = tasks.length;
        const resolved = tasks.filter(t => t.done).length;
        const pending = total - resolved;

        statTotal.textContent = pad(total);
        statPending.textContent = pad(pending);
        statDone.textContent = pad(resolved);

        if (countAll) countAll.textContent = total;
        if (countPending) countPending.textContent = pending;
        if (countDone) countDone.textContent = resolved;
    }

    // Add new directive
    form.addEventListener('submit', async (e) => {
        e.preventDefault();
        const title = titleInput.value.trim();
        if (!title) return;

        const priority = document.querySelector('input[name="priority"]:checked').value;

        try {
            const res = await fetch('/api/tasks', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ title, priority })
            });

            if (res.ok) {
                titleInput.value = '';
                await loadTasks();
            }
        } catch (err) {
            console.error('Failed to commit directive:', err);
        }
    });

    // Toggle directive resolution state
    async function toggleTask(id) {
        try {
            const res = await fetch(`/api/tasks/${id}/toggle`, { method: 'POST' });
            if (res.ok) {
                await loadTasks();
            }
        } catch (err) {
            console.error('Toggle error:', err);
        }
    }

    // Delete directive
    async function deleteTask(id) {
        try {
            const res = await fetch(`/api/tasks/${id}`, { method: 'DELETE' });
            if (res.ok) {
                await loadTasks();
            }
        } catch (err) {
            console.error('Purge error:', err);
        }
    }

    // Scope switcher navigation
    scopeButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            scopeButtons.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            currentFilter = btn.dataset.filter;
            renderLedger();
        });
    });

    function escapeHtml(str) {
        const div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    // Initial sync
    loadTasks();
});
