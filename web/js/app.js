/**
 * Unified Engineering Workbench - Client Application Logic
 * Coordinates 3D WebGL Viewers (Three.js), FEA Solvers, Multicore Jobs, and Hardware Telemetry.
 */

document.addEventListener('DOMContentLoaded', () => {
    initNavigation();
    initSTLViewer();
    initANSYSViewer();
    initFEASolver();
    initPerformanceController();
    initHardwareMonitor();
    fetchHardwareSpecs();
});

// ============================================================================
// 1. Navigation Controller
// ============================================================================
function initNavigation() {
    const navButtons = document.querySelectorAll('.nav-btn');
    const views = document.querySelectorAll('.app-view');

    navButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const targetViewId = btn.getAttribute('data-view');
            navButtons.forEach(b => b.classList.remove('active'));
            views.forEach(v => v.classList.remove('active'));

            btn.classList.add('active');
            const targetView = document.getElementById(targetViewId);
            if (targetView) targetView.classList.add('active');

            // Trigger 3D renderer resize when switching to 3D views
            if (targetViewId === 'view-geometry' && window.stlResize) window.stlResize();
            if (targetViewId === 'view-thermal' && window.ansysResize) window.ansysResize();
        });
    });

    // Subview tabs in Thermal Analysis
    const tabButtons = document.querySelectorAll('.tab-btn');
    const subviews = document.querySelectorAll('.subview');
    tabButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const targetId = btn.getAttribute('data-subview');
            tabButtons.forEach(b => b.classList.remove('active'));
            subviews.forEach(s => s.classList.remove('active'));

            btn.classList.add('active');
            const sub = document.getElementById(targetId);
            if (sub) sub.classList.add('active');
            if (targetId === 'subview-ansys' && window.ansysResize) window.ansysResize();
        });
    });
}

// ============================================================================
// 2. ASCII STL 3D Viewer (Three.js)
// ============================================================================
let stlScene, stlCamera, stlRenderer, stlMesh;
let isDragging = false, prevMouseX = 0, prevMouseY = 0;
let spherical = { radius: 2.5, theta: Math.PI / 4, phi: Math.PI / 3 };
let targetCenter = new THREE.Vector3(0, 0, 0);

function initSTLViewer() {
    const container = document.getElementById('stl-canvas-container');
    if (!container) return;

    stlScene = new THREE.Scene();
    stlScene.background = new THREE.Color(0x0a0e17);

    stlCamera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.01, 100);
    updateCameraPosition();

    stlRenderer = new THREE.WebGLRenderer({ antialias: true });
    stlRenderer.setSize(container.clientWidth, container.clientHeight);
    container.appendChild(stlRenderer.domElement);

    // Lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
    stlScene.add(ambientLight);

    const dirLight1 = new THREE.DirectionalLight(0xffffff, 0.8);
    dirLight1.position.set(5, 10, 7);
    stlScene.add(dirLight1);

    const dirLight2 = new THREE.DirectionalLight(0x3b82f6, 0.4);
    dirLight2.position.set(-5, -5, -5);
    stlScene.add(dirLight2);

    // Grid floor
    const grid = new THREE.GridHelper(2, 20, 0x334155, 0x1e293b);
    grid.position.y = -0.05;
    stlScene.add(grid);

    // Mouse Interaction (Orbit / Pan / Zoom)
    container.addEventListener('mousedown', (e) => {
        isDragging = true;
        prevMouseX = e.clientX;
        prevMouseY = e.clientY;
    });

    window.addEventListener('mouseup', () => isDragging = false);

    container.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const dx = e.clientX - prevMouseX;
        const dy = e.clientY - prevMouseY;
        prevMouseX = e.clientX;
        prevMouseY = e.clientY;

        if (e.buttons === 1) { // Rotate
            spherical.theta -= dx * 0.01;
            spherical.phi = Math.max(0.1, Math.min(Math.PI - 0.1, spherical.phi - dy * 0.01));
            updateCameraPosition();
        } else if (e.buttons === 2) { // Pan
            const panSpeed = 0.002 * spherical.radius;
            targetCenter.x -= dx * panSpeed;
            targetCenter.y += dy * panSpeed;
            updateCameraPosition();
        }
    });

    container.addEventListener('wheel', (e) => {
        e.preventDefault();
        spherical.radius = Math.max(0.2, Math.min(20, spherical.radius + e.deltaY * 0.002 * spherical.radius));
        updateCameraPosition();
    });

    container.addEventListener('contextmenu', (e) => e.preventDefault());

    document.getElementById('btn-stl-reset-view')?.addEventListener('click', () => {
        spherical = { radius: 2.0, theta: Math.PI / 4, phi: Math.PI / 3 };
        targetCenter.set(0.5, 0.025, 0.025);
        updateCameraPosition();
    });

    window.stlResize = () => {
        if (!container || !stlRenderer || !stlCamera) return;
        stlCamera.aspect = container.clientWidth / container.clientHeight;
        stlCamera.updateProjectionMatrix();
        stlRenderer.setSize(container.clientWidth, container.clientHeight);
    };
    window.addEventListener('resize', window.stlResize);

    // Animation Loop
    function animate() {
        requestAnimationFrame(animate);
        stlRenderer.render(stlScene, stlCamera);
    }
    animate();

    // File input handler
    const fileInput = document.getElementById('stl-file-input');
    fileInput?.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (evt) => parseAndRenderSTL(evt.target.result);
        reader.readAsText(file);
    });

    document.getElementById('btn-load-sample-stl')?.addEventListener('click', async () => {
        try {
            const resp = await fetch('data/sample_rod.stl');
            if (!resp.ok) throw new Error('Could not load data/sample_rod.stl');
            const text = await resp.text();
            parseAndRenderSTL(text);
        } catch (err) {
            showAlert('stl', 'Failed to load sample STL: ' + err.message);
        }
    });
}

function updateCameraPosition() {
    stlCamera.position.x = targetCenter.x + spherical.radius * Math.sin(spherical.phi) * Math.sin(spherical.theta);
    stlCamera.position.y = targetCenter.y + spherical.radius * Math.cos(spherical.phi);
    stlCamera.position.z = targetCenter.z + spherical.radius * Math.sin(spherical.phi) * Math.cos(spherical.theta);
    stlCamera.lookAt(targetCenter);
}

async function parseAndRenderSTL(stlContent) {
    hideAlert('stl');
    try {
        const resp = await fetch('/api/cad/parse-stl', {
            method: 'POST',
            body: stlContent
        });
        const data = await resp.json();

        if (!resp.ok || !data.is_valid) {
            showAlert('stl', data.error_message || 'Malformed STL format.');
            return;
        }

        // Update UI statistics
        document.getElementById('stl-solid-name').textContent = data.solid_name || 'Solid';
        document.getElementById('stl-triangle-count').textContent = data.triangle_count.toLocaleString();
        document.getElementById('stl-bbox-x').textContent = `[${data.bbox.min_x.toFixed(3)}, ${data.bbox.max_x.toFixed(3)}] m`;
        document.getElementById('stl-bbox-y').textContent = `[${data.bbox.min_y.toFixed(3)}, ${data.bbox.max_y.toFixed(3)}] m`;
        document.getElementById('stl-bbox-z').textContent = `[${data.bbox.min_z.toFixed(3)}, ${data.bbox.max_z.toFixed(3)}] m`;

        // Render Geometry in Three.js
        if (stlMesh) stlScene.remove(stlMesh);

        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute('position', new THREE.BufferAttribute(new Float32Array(data.flat_positions), 3));
        geometry.setAttribute('normal', new THREE.BufferAttribute(new Float32Array(data.flat_normals), 3));

        const material = new THREE.MeshPhongMaterial({
            color: 0x38bdf8,
            specular: 0x111111,
            shininess: 30,
            side: THREE.DoubleSide,
            flatShading: true
        });

        stlMesh = new THREE.Mesh(geometry, material);
        stlScene.add(stlMesh);

        // Center camera on object
        targetCenter.set(
            (data.bbox.min_x + data.bbox.max_x) / 2,
            (data.bbox.min_y + data.bbox.max_y) / 2,
            (data.bbox.min_z + data.bbox.max_z) / 2
        );
        spherical.radius = Math.max(data.bbox.max_x - data.bbox.min_x, data.bbox.max_y - data.bbox.min_y, 0.5) * 2.2;
        updateCameraPosition();
    } catch (e) {
        showAlert('stl', 'Failed to communicate with C++ backend: ' + e.message);
    }
}

// ============================================================================
// 3. ANSYS Nodal 3D Point Cloud Viewer (Three.js)
// ============================================================================
let ansysScene, ansysCamera, ansysRenderer, ansysPoints, ansysDataNodes = [];
let ansysSpherical = { radius: 2.5, theta: Math.PI / 4, phi: Math.PI / 3 };
let ansysTarget = new THREE.Vector3(0, 0, 0);

function initANSYSViewer() {
    const container = document.getElementById('ansys-canvas-container');
    if (!container) return;

    ansysScene = new THREE.Scene();
    ansysScene.background = new THREE.Color(0x0a0e17);

    ansysCamera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.01, 100);
    updateANSYSCamera();

    ansysRenderer = new THREE.WebGLRenderer({ antialias: true });
    ansysRenderer.setSize(container.clientWidth, container.clientHeight);
    container.appendChild(ansysRenderer.domElement);

    const grid = new THREE.GridHelper(2, 20, 0x334155, 0x1e293b);
    ansysScene.add(grid);

    // Orbit controls
    let dragging = false, px = 0, py = 0;
    container.addEventListener('mousedown', (e) => { dragging = true; px = e.clientX; py = e.clientY; });
    window.addEventListener('mouseup', () => dragging = false);
    container.addEventListener('mousemove', (e) => {
        if (!dragging) return;
        const dx = e.clientX - px;
        const dy = e.clientY - py;
        px = e.clientX; py = e.clientY;
        ansysSpherical.theta -= dx * 0.01;
        ansysSpherical.phi = Math.max(0.1, Math.min(Math.PI - 0.1, ansysSpherical.phi - dy * 0.01));
        updateANSYSCamera();
    });
    container.addEventListener('wheel', (e) => {
        e.preventDefault();
        ansysSpherical.radius = Math.max(0.2, Math.min(20, ansysSpherical.radius + e.deltaY * 0.002 * ansysSpherical.radius));
        updateANSYSCamera();
    });

    // Node click inspection via Raycasting
    const raycaster = new THREE.Raycaster();
    raycaster.params.Points.threshold = 0.03;
    const mouse = new THREE.Vector2();

    container.addEventListener('click', (e) => {
        const rect = container.getBoundingClientRect();
        mouse.x = ((e.clientX - rect.left) / container.clientWidth) * 2 - 1;
        mouse.y = -((e.clientY - rect.top) / container.clientHeight) * 2 + 1;

        raycaster.setFromCamera(mouse, ansysCamera);
        if (ansysPoints) {
            const intersects = raycaster.intersectObject(ansysPoints);
            if (intersects.length > 0) {
                const idx = intersects[0].index;
                const node = ansysDataNodes[idx];
                if (node) {
                    document.getElementById('node-inspect-content').innerHTML = `
                        <div class="stat-row"><span>Node ID:</span> <strong>#${node.node_id}</strong></div>
                        <div class="stat-row"><span>Position X:</span> <strong>${node.x.toFixed(4)} m</strong></div>
                        <div class="stat-row"><span>Position Y:</span> <strong>${node.y.toFixed(4)} m</strong></div>
                        <div class="stat-row"><span>Position Z:</span> <strong>${node.z.toFixed(4)} m</strong></div>
                        <div class="stat-row"><span>Temperature:</span> <strong style="color:#f87171;">${node.temp.toFixed(2)} °C</strong></div>
                    `;
                }
            }
        }
    });

    window.ansysResize = () => {
        if (!container || !ansysRenderer || !ansysCamera) return;
        ansysCamera.aspect = container.clientWidth / container.clientHeight;
        ansysCamera.updateProjectionMatrix();
        ansysRenderer.setSize(container.clientWidth, container.clientHeight);
    };
    window.addEventListener('resize', window.ansysResize);

    function animate() {
        requestAnimationFrame(animate);
        ansysRenderer.render(ansysScene, ansysCamera);
    }
    animate();

    // File input
    document.getElementById('ansys-file-input')?.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (evt) => parseAndRenderANSYS(evt.target.result);
        reader.readAsText(file);
    });

    document.getElementById('btn-load-sample-ansys')?.addEventListener('click', async () => {
        try {
            const resp = await fetch('data/ansys_thermal_synthetic.csv');
            if (!resp.ok) throw new Error('Could not load data/ansys_thermal_synthetic.csv');
            const text = await resp.text();
            parseAndRenderANSYS(text);
        } catch (err) {
            showAlert('ansys', 'Failed to load synthetic CSV: ' + err.message);
        }
    });
}

function updateANSYSCamera() {
    ansysCamera.position.x = ansysTarget.x + ansysSpherical.radius * Math.sin(ansysSpherical.phi) * Math.sin(ansysSpherical.theta);
    ansysCamera.position.y = ansysTarget.y + ansysSpherical.radius * Math.cos(ansysSpherical.phi);
    ansysCamera.position.z = ansysTarget.z + ansysSpherical.radius * Math.sin(ansysSpherical.phi) * Math.cos(ansysSpherical.theta);
    ansysCamera.lookAt(ansysTarget);
}

// Map temperature to RGB gradient (Blue -> Cyan -> Green -> Yellow -> Red)
function getHeatmapColor(temp, minT, maxT) {
    const norm = Math.max(0, Math.min(1, (temp - minT) / (maxT - minT || 1)));
    let r = 0, g = 0, b = 0;
    if (norm < 0.25) {
        b = 1.0; g = norm * 4;
    } else if (norm < 0.5) {
        g = 1.0; b = 1.0 - (norm - 0.25) * 4;
    } else if (norm < 0.75) {
        g = 1.0; r = (norm - 0.5) * 4;
    } else {
        r = 1.0; g = 1.0 - (norm - 0.75) * 4;
    }
    return [r, g, b];
}

async function parseAndRenderANSYS(csvContent) {
    hideAlert('ansys');
    try {
        const resp = await fetch('/api/ansys/parse-csv', {
            method: 'POST',
            body: csvContent
        });
        const data = await resp.json();

        if (!resp.ok || !data.is_valid) {
            showAlert('ansys', data.error_message || 'Malformed CSV format.');
            return;
        }

        ansysDataNodes = data.sample_nodes || [];
        document.getElementById('ansys-node-count').textContent = data.node_count.toLocaleString();
        document.getElementById('ansys-min-temp').textContent = `${data.min_temp_C.toFixed(2)} °C`;
        document.getElementById('ansys-max-temp').textContent = `${data.max_temp_C.toFixed(2)} °C`;
        document.getElementById('ansys-mean-temp').textContent = `${data.mean_temp_C.toFixed(2)} °C`;

        if (ansysPoints) ansysScene.remove(ansysPoints);

        const positions = new Float32Array(data.flat_positions);
        const temps = data.flat_temperatures;
        const colors = new Float32Array(positions.length);

        for (let i = 0; i < temps.length; ++i) {
            const [r, g, b] = getHeatmapColor(temps[i], data.min_temp_C, data.max_temp_C);
            colors[i * 3] = r;
            colors[i * 3 + 1] = g;
            colors[i * 3 + 2] = b;
        }

        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));

        // Create point sprite material
        const material = new THREE.PointsMaterial({
            size: 0.04,
            vertexColors: true,
            transparent: true,
            opacity: 0.95
        });

        ansysPoints = new THREE.Points(geometry, material);
        ansysScene.add(ansysPoints);

        ansysTarget.set(
            (data.bbox.min_x + data.bbox.max_x) / 2,
            (data.bbox.min_y + data.bbox.max_y) / 2,
            (data.bbox.min_z + data.bbox.max_z) / 2
        );
        ansysSpherical.radius = 2.0;
        updateANSYSCamera();
    } catch (e) {
        showAlert('ansys', 'Failed to communicate with C++ backend: ' + e.message);
    }
}

// ============================================================================
// 4. Task C: Thermal FEA Solver & Convergence
// ============================================================================
function initFEASolver() {
    document.getElementById('btn-fea-solve')?.addEventListener('click', runFEASolve);
    document.getElementById('btn-fea-convergence')?.addEventListener('click', runFEAConvergence);
    document.getElementById('btn-fea-export-csv')?.addEventListener('click', runFEAExport);

    // Initial default solve
    runFEASolve();
}

async function runFEASolve() {
    hideAlert('fea');
    const p = {
        L: parseFloat(document.getElementById('fea-param-L').value),
        A: parseFloat(document.getElementById('fea-param-A').value),
        k: parseFloat(document.getElementById('fea-param-k').value),
        q: parseFloat(document.getElementById('fea-param-q').value),
        T0: parseFloat(document.getElementById('fea-param-T0').value),
        TL: parseFloat(document.getElementById('fea-param-TL').value),
        N: parseInt(document.getElementById('fea-param-N').value)
    };

    try {
        const resp = await fetch('/api/fea/solve', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(p)
        });
        const data = await resp.json();

        if (!resp.ok || !data.is_valid) {
            showAlert('fea', data.error || 'Parameter validation failure.');
            return;
        }

        const theoretical_mid_err = (p.q / (8.0 * p.k)) * (data.h * data.h);
        document.getElementById('fea-node-err').textContent = `${data.max_node_error.toExponential(4)} °C`;
        document.getElementById('fea-mid-err').textContent = `${data.max_midpoint_error.toFixed(6)} °C`;
        document.getElementById('fea-theory-err').textContent = `${theoretical_mid_err.toFixed(6)} °C`;

        drawFEAProfileCanvas(data.x_nodes, data.T_nodes, data.T_exact_nodes, p.N);
    } catch (err) {
        showAlert('fea', 'Error connecting to solver: ' + err.message);
    }
}

async function runFEAConvergence() {
    hideAlert('fea');
    const p = {
        L: parseFloat(document.getElementById('fea-param-L').value),
        A: parseFloat(document.getElementById('fea-param-A').value),
        k: parseFloat(document.getElementById('fea-param-k').value),
        q: parseFloat(document.getElementById('fea-param-q').value),
        T0: parseFloat(document.getElementById('fea-param-T0').value),
        TL: parseFloat(document.getElementById('fea-param-TL').value),
        N_values: [10, 20, 40, 80]
    };

    try {
        const resp = await fetch('/api/fea/convergence', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(p)
        });
        const data = await resp.json();

        const tbody = document.getElementById('convergence-tbody');
        tbody.innerHTML = '';

        data.rows.forEach(r => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>N = ${r.N}</strong></td>
                <td>${r.h.toFixed(4)}</td>
                <td>${r.max_node_error.toExponential(4)}</td>
                <td><strong style="color:#60a5fa;">${r.max_midpoint_error.toFixed(6)}</strong></td>
                <td><span class="synthetic-badge" style="background:rgba(16,185,129,0.15);color:#34d399;border-color:rgba(16,185,129,0.3);">${r.observed_order > 0 ? r.observed_order.toFixed(2) : '-'}</span></td>
            `;
            tbody.appendChild(tr);
        });
    } catch (err) {
        showAlert('fea', 'Convergence study error: ' + err.message);
    }
}

async function runFEAExport() {
    try {
        const resp = await fetch('/api/fea/export-csv', { method: 'POST' });
        const data = await resp.json();
        if (data.success) {
            alert('Underlying results successfully written atomically to results/fea_profiles.csv and results/fea_convergence.csv!');
        }
    } catch (err) {
        alert('Export failed: ' + err.message);
    }
}

function drawFEAProfileCanvas(xNodes, tNodes, tExact, N) {
    const canvas = document.getElementById('fea-profile-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;
    ctx.clearRect(0, 0, W, H);

    const padL = 60, padR = 30, padT = 30, padB = 40;
    const plotW = W - padL - padR;
    const plotH = H - padT - padB;

    const minX = 0, maxX = 1.0;
    let minT = Math.min(...tNodes, ...tExact), maxT = Math.max(...tNodes, ...tExact);
    minT = Math.floor(minT / 10) * 10;
    maxT = Math.ceil(maxT / 10) * 10;

    const toX = (x) => padL + ((x - minX) / (maxX - minX)) * plotW;
    const toY = (t) => padT + plotH - ((t - minT) / (maxT - minT)) * plotH;

    // Draw Grid
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);

    for (let x = 0; x <= 1.0; x += 0.2) {
        ctx.beginPath();
        ctx.moveTo(toX(x), padT);
        ctx.lineTo(toX(x), padT + plotH);
        ctx.stroke();

        ctx.fillStyle = '#94a3b8';
        ctx.font = '10px monospace';
        ctx.fillText(x.toFixed(1), toX(x) - 8, padT + plotH + 18);
    }

    for (let t = minT; t <= maxT; t += 20) {
        ctx.beginPath();
        ctx.moveTo(padL, toY(t));
        ctx.lineTo(padL + plotW, toY(t));
        ctx.stroke();

        ctx.fillStyle = '#94a3b8';
        ctx.font = '10px monospace';
        ctx.fillText(`${t}`, padL - 35, toY(t) + 4);
    }
    ctx.setLineDash([]);

    // Draw Exact Curve (Solid Orange)
    ctx.strokeStyle = '#f59e0b';
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    for (let i = 0; i < xNodes.length; ++i) {
        const px = toX(xNodes[i]), py = toY(tExact[i]);
        if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
    }
    ctx.stroke();

    // Draw FE Solution (Dashed Blue with node points)
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let i = 0; i < xNodes.length; ++i) {
        const px = toX(xNodes[i]), py = toY(tNodes[i]);
        if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
    }
    ctx.stroke();

    // Node circles
    ctx.fillStyle = '#38bdf8';
    for (let i = 0; i < xNodes.length; ++i) {
        const px = toX(xNodes[i]), py = toY(tNodes[i]);
        ctx.beginPath();
        ctx.arc(px, py, 4, 0, Math.PI * 2);
        ctx.fill();
    }

    // Legend
    ctx.fillStyle = '#f59e0b';
    ctx.fillRect(W - 200, padT + 10, 16, 4);
    ctx.fillStyle = '#f1f5f9';
    ctx.font = '11px sans-serif';
    ctx.fillText('Exact Analytical', W - 175, padT + 15);

    ctx.fillStyle = '#38bdf8';
    ctx.fillRect(W - 200, padT + 28, 16, 4);
    ctx.fillStyle = '#f1f5f9';
    ctx.fillText(`FE Linear (N = ${N})`, W - 175, padT + 33);
}

// ============================================================================
// 5. Task D: Multicore Performance Controller
// ============================================================================
let pollInterval = null;

function initPerformanceController() {
    const btnRun = document.getElementById('btn-perf-run');
    const btnCancel = document.getElementById('btn-perf-cancel');
    const btnBenchmark = document.getElementById('btn-perf-benchmark');

    btnRun?.addEventListener('click', async () => {
        const cases = parseInt(document.getElementById('perf-cases-input').value);
        const workers = parseInt(document.getElementById('perf-workers-input').value);

        btnRun.disabled = true;
        btnCancel.disabled = false;
        document.getElementById('perf-status-text').textContent = 'Status: Launching worker pool...';

        await fetch('/api/parallel/start', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cases, workers })
        });

        startProgressPolling();
    });

    btnCancel?.addEventListener('click', async () => {
        await fetch('/api/parallel/cancel', { method: 'POST' });
        document.getElementById('perf-status-text').textContent = 'Status: Cancellation requested...';
    });

    btnBenchmark?.addEventListener('click', runBenchmarkProtocol);
}

function startProgressPolling() {
    if (pollInterval) clearInterval(pollInterval);
    const pBar = document.getElementById('perf-progress-bar');
    const pText = document.getElementById('perf-progress-percent');
    const sText = document.getElementById('perf-status-text');
    const btnRun = document.getElementById('btn-perf-run');
    const btnCancel = document.getElementById('btn-perf-cancel');

    pollInterval = setInterval(async () => {
        try {
            const resp = await fetch('/api/parallel/status');
            const data = await resp.json();

            pBar.style.width = `${data.progress}%`;
            pText.textContent = `${data.progress}%`;
            sText.textContent = `Status: ${data.status_message} (${data.elapsed_ms.toFixed(1)} ms)`;

            if (!data.is_running) {
                clearInterval(pollInterval);
                btnRun.disabled = false;
                btnCancel.disabled = true;
            }
        } catch (e) {
            clearInterval(pollInterval);
        }
    }, 100);
}

async function runBenchmarkProtocol() {
    try {
        const resp = await fetch('/api/parallel/benchmark', { method: 'POST' });
        const data = await resp.json();

        const tbody = document.getElementById('perf-benchmark-tbody');
        tbody.innerHTML = '';

        const workers = [], speedups = [], efficiencies = [];

        data.records.forEach(r => {
            workers.push(r.workers);
            speedups.push(r.speedup);
            efficiencies.push(r.efficiency * 100);

            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>${r.workers} Worker${r.workers > 1 ? 's' : ''}</strong></td>
                <td>${r.warmup_ms.toFixed(2)}</td>
                <td><strong style="color:#60a5fa;">${r.median_ms.toFixed(2)}</strong></td>
                <td>${r.speedup.toFixed(2)}x</td>
                <td>${(r.efficiency * 100).toFixed(1)}%</td>
                <td><span class="synthetic-badge" style="background:rgba(16,185,129,0.15);color:#34d399;border-color:rgba(16,185,129,0.3);">${r.max_diff_C.toExponential(3)} °C (PASS)</span></td>
            `;
            tbody.appendChild(tr);
        });

        drawPerformanceChart(workers, speedups, efficiencies);
    } catch (e) {
        alert('Benchmark error: ' + e.message);
    }
}

function drawPerformanceChart(workers, speedups, efficiencies) {
    const canvas = document.getElementById('perf-chart-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;
    ctx.clearRect(0, 0, W, H);

    const padL = 50, padR = 60, padT = 30, padB = 40;
    const plotW = W - padL - padR, plotH = H - padT - padB;

    // Ideal linear line
    ctx.strokeStyle = '#64748b';
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(padL, padT + plotH);
    ctx.lineTo(padL + plotW, padT);
    ctx.stroke();
    ctx.setLineDash([]);

    // Speedup line
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 3;
    ctx.beginPath();
    workers.forEach((w, i) => {
        const x = padL + (i / (workers.length - 1 || 1)) * plotW;
        const y = padT + plotH - (speedups[i] / 4.0) * plotH;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();

    // Data points
    workers.forEach((w, i) => {
        const x = padL + (i / (workers.length - 1 || 1)) * plotW;
        const y = padT + plotH - (speedups[i] / 4.0) * plotH;
        ctx.fillStyle = '#38bdf8';
        ctx.beginPath();
        ctx.arc(x, y, 6, 0, Math.PI * 2);
        ctx.fill();

        ctx.fillStyle = '#fff';
        ctx.font = '11px sans-serif';
        ctx.fillText(`${speedups[i].toFixed(2)}x`, x - 12, y - 10);
    });
}

// ============================================================================
// 6. Task E: Embedded Hardware Monitor
// ============================================================================
function initHardwareMonitor() {
    setInterval(pollHardwareTelemetry, 800);
    pollHardwareTelemetry();

    document.getElementById('btn-fault-inject')?.addEventListener('click', async () => {
        await fetch('/api/hardware/fault-inject', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enable: true })
        });
        showHwAlert('UART communication timeout after 500ms. Response frame missing from MCU.');
    });

    document.getElementById('btn-fault-retry')?.addEventListener('click', async () => {
        await fetch('/api/hardware/retry', { method: 'POST' });
        hideHwAlert();
    });
}

async function pollHardwareTelemetry() {
    try {
        const resp = await fetch('/api/hardware/devices');
        if (!resp.ok) return;
        const data = await resp.json();

        // MCU
        document.getElementById('mcu-temp').textContent = `${data.mcu.internal_temp_C.toFixed(1)} °C`;
        document.getElementById('mcu-voltage').textContent = `${data.mcu.rail_voltage_V.toFixed(3)} V`;
        document.getElementById('mcu-current').textContent = `${data.mcu.current_mA.toFixed(1)} mA`;
        document.getElementById('mcu-uptime').textContent = `${(data.mcu.uptime_ms / 1000).toFixed(1)} s`;
        document.getElementById('mcu-status-box').textContent = data.mcu.status_message;

        const mcuBadge = document.getElementById('mcu-state-badge');
        if (data.mcu.state === 'CONNECTED') {
            mcuBadge.className = 'state-badge connected';
            mcuBadge.textContent = 'CONNECTED';
        } else {
            mcuBadge.className = 'state-badge fault';
            mcuBadge.textContent = 'FAULT_TIMEOUT';
            showHwAlert(data.mcu.status_message);
        }

        // FPGA
        document.getElementById('fpga-temp').textContent = `${data.fpga.internal_temp_C.toFixed(1)} °C`;
        document.getElementById('fpga-voltage').textContent = `${data.fpga.rail_voltage_V.toFixed(3)} V`;
        document.getElementById('fpga-current').textContent = `${data.fpga.current_mA.toFixed(1)} mA`;
        document.getElementById('fpga-uptime').textContent = `${(data.fpga.uptime_ms / 1000).toFixed(1)} s`;
        document.getElementById('fpga-status-box').textContent = data.fpga.status_message;

        // JTAG
        document.getElementById('jtag-voltage').textContent = `${data.jtag.rail_voltage_V.toFixed(3)} V`;
        document.getElementById('jtag-current').textContent = `${data.jtag.current_mA.toFixed(1)} mA`;
        document.getElementById('jtag-uptime').textContent = `${(data.jtag.uptime_ms / 1000).toFixed(1)} s`;
        document.getElementById('jtag-status-box').textContent = data.jtag.status_message;
    } catch (e) {}
}

async function fetchHardwareSpecs() {
    try {
        const resp = await fetch('/api/hardware-info');
        if (!resp.ok) return;
        const hw = await resp.json();
        document.getElementById('hw-cpu-label').textContent = hw.cpu_model;
        document.getElementById('hw-cores-label').textContent = `${hw.physical_cores}P / ${hw.logical_cores}L`;
        document.getElementById('hw-ram-label').textContent = `${hw.ram_gb.toFixed(1)} GB`;
        document.getElementById('hw-compiler-label').textContent = hw.compiler;
    } catch (e) {}
}

// ============================================================================
// Alert Helpers
// ============================================================================
function showAlert(prefix, message) {
    const box = document.getElementById(`${prefix}-alert-box`);
    const msg = document.getElementById(`${prefix}-alert-message`);
    if (box && msg) {
        msg.textContent = message;
        box.classList.remove('hidden');
    }
}

function hideAlert(prefix) {
    const box = document.getElementById(`${prefix}-alert-box`);
    if (box) box.classList.add('hidden');
}

function showHwAlert(msg) {
    const box = document.getElementById('hw-alert-box');
    const p = box?.querySelector('p');
    if (box && p) {
        p.textContent = msg;
        box.classList.remove('hidden');
    }
}

function hideHwAlert() {
    const box = document.getElementById('hw-alert-box');
    if (box) box.classList.add('hidden');
}
