// Gource Web - JavaScript Interface

let gourceModule = null;
let gourceLoadLog = null;

// Setup canvas
const canvas = document.getElementById('canvas');
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

// Enable keyboard input on canvas
canvas.tabIndex = 1;
canvas.style.outline = 'none';
canvas.addEventListener('click', () => canvas.focus());
canvas.focus();

// Initialize Gource when module is ready
GourceModule({
    canvas: canvas,
    print: (text) => console.log(text),
    printErr: (text) => console.error(text)
}).then(module => {
    gourceModule = module;

    // Get exported functions
    gourceLoadLog = module.cwrap('gource_load_log', 'number', ['string']);

    // Hide loading indicator
    document.getElementById('loading').classList.add('hidden');

    console.log('Gource initialized');
}).catch(err => {
    console.error('Failed to initialize Gource:', err);
    document.getElementById('loading').innerHTML = '<p style="color: #f44;">Failed to load Gource</p>';
});

// Load log file
document.getElementById('loadBtn').addEventListener('click', () => {
    document.getElementById('logFile').click();
});

document.getElementById('logFile').addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
        const logData = event.target.result;
        if (gourceLoadLog) {
            gourceLoadLog(logData);
        }
    };
    reader.readAsText(file);
});

// Fullscreen toggle
document.getElementById('fullscreenBtn').addEventListener('click', () => {
    const canvas = document.getElementById('canvas');
    if (!document.fullscreenElement) {
        canvas.requestFullscreen().catch(err => {
            console.warn('Fullscreen request failed:', err);
        });
    } else {
        document.exitFullscreen();
    }
});

// Resize handling
window.addEventListener('resize', () => {
    const canvas = document.getElementById('canvas');
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
});

// Initial resize
window.dispatchEvent(new Event('resize'));

// Fetch log from URL
async function loadLogFromUrl(url) {
    try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const logData = await response.text();
        if (gourceLoadLog) {
            gourceLoadLog(logData);
            // Focus canvas for keyboard input
            canvas.focus();
        }
    } catch (err) {
        console.error('Failed to load log from URL:', err);
    }
}

// Export for external use
window.GourceWeb = {
    loadLog: (data) => gourceLoadLog && gourceLoadLog(data),
    loadLogFromUrl: loadLogFromUrl
};
