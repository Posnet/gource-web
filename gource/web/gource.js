// Gource Web - JavaScript Interface

let gourceModule = null;
let gourceLoadLog = null;
let gourceReset = null;
let gourcePause = null;
let gourceResume = null;

// GitHub Auth State
let githubToken = localStorage.getItem('github_token');
let githubUser = null;

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
    gourceReset = module.cwrap('gource_reset', null, []);
    gourcePause = module.cwrap('gource_pause', null, []);
    gourceResume = module.cwrap('gource_resume', null, []);

    // Hide loading indicator
    document.getElementById('loading').classList.add('hidden');

    console.log('Gource initialized');
}).catch(err => {
    console.error('Failed to initialize Gource:', err);
    document.getElementById('loading').innerHTML = '<p style="color: #f44;">Failed to load Gource</p>';
});

// Load log file modal
document.getElementById('loadBtn').addEventListener('click', () => {
    openModal('loadLogModal');
});

document.getElementById('loadLogModalClose').addEventListener('click', () => {
    closeModal('loadLogModal');
});

document.getElementById('loadLogModal').addEventListener('click', (e) => {
    if (e.target.id === 'loadLogModal') {
        closeModal('loadLogModal');
    }
});

// CLI command copy to clipboard
document.getElementById('cliCommand').addEventListener('click', async () => {
    const command = 'git log --pretty=format:"%at|%an|%ae" --reverse --name-status';
    try {
        await navigator.clipboard.writeText(command);
        const hint = document.querySelector('#cliCommand .copy-hint');
        hint.textContent = 'Copied!';
        setTimeout(() => { hint.textContent = 'Click to copy'; }, 2000);
    } catch (err) {
        console.error('Failed to copy:', err);
    }
});

// Select file button in modal
document.getElementById('selectFileBtn').addEventListener('click', () => {
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
            closeModal('loadLogModal');
        }
    };
    reader.readAsText(file);
});

// Resize handling
window.addEventListener('resize', () => {
    const canvas = document.getElementById('canvas');
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
});

// Initial resize
window.dispatchEvent(new Event('resize'));

// ============================================================================
// Modal Management (with input capture control)
// ============================================================================

let activeModals = 0;

function openModal(modalId) {
    const modal = document.getElementById(modalId);
    modal.classList.add('show');
    activeModals++;
    // Disable canvas input capture when modal is open
    canvas.style.pointerEvents = 'none';
    canvas.blur();
}

function closeModal(modalId) {
    const modal = document.getElementById(modalId);
    modal.classList.remove('show');
    activeModals--;
    if (activeModals <= 0) {
        activeModals = 0;
        // Re-enable canvas input capture when all modals closed
        canvas.style.pointerEvents = 'auto';
        canvas.focus();
    }
}

function isModalOpen() {
    return activeModals > 0;
}

// Export modal functions globally for inline scripts
window.openModal = openModal;
window.closeModal = closeModal;
window.isModalOpen = isModalOpen;

// Fetch log from URL
async function loadLogFromUrl(url) {
    try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const logData = await response.text();
        if (gourceLoadLog) {
            gourceLoadLog(logData);
            canvas.focus();
        }
    } catch (err) {
        console.error('Failed to load log from URL:', err);
    }
}

// ============================================================================
// GitHub OAuth & API
// ============================================================================

function updateGitHubButton() {
    const btn = document.getElementById('githubBtn');
    const text = document.getElementById('githubBtnText');

    if (githubToken && githubUser) {
        text.textContent = 'Select Repo';
        btn.classList.add('logged-in');
        btn.title = `Logged in as ${githubUser.login}`;
    } else if (githubToken) {
        text.textContent = 'Loading...';
    } else {
        text.textContent = 'Login with GitHub';
        btn.classList.remove('logged-in');
        btn.title = '';
    }
}

// Check for OAuth callback token in URL hash
function handleOAuthCallback() {
    const hash = window.location.hash;
    if (hash.includes('access_token=')) {
        const params = new URLSearchParams(hash.substring(1));
        const token = params.get('access_token');
        if (token) {
            githubToken = token;
            localStorage.setItem('github_token', token);
            // Clear hash from URL
            history.replaceState(null, '', window.location.pathname);
            fetchGitHubUser();
        }
    }
}

async function fetchGitHubUser() {
    if (!githubToken) return;

    try {
        const response = await fetch('/api/user', {
            headers: { 'Authorization': `Bearer ${githubToken}` }
        });

        if (response.ok) {
            githubUser = await response.json();
            updateGitHubButton();
        } else if (response.status === 401) {
            // Token expired or invalid
            logout();
        }
    } catch (err) {
        console.error('Failed to fetch user:', err);
    }
}

function logout() {
    githubToken = null;
    githubUser = null;
    localStorage.removeItem('github_token');
    updateGitHubButton();
}

// GitHub button click handler
document.getElementById('githubBtn').addEventListener('click', async () => {
    if (githubToken && githubUser) {
        // Already logged in - show repo selector with user's repos
        showRepoModal(true);
    } else {
        // Not logged in - show modal with just URL input
        showRepoModal(false);
    }
});

// Parse GitHub URL to extract owner/repo
function parseGitHubUrl(url) {
    // Handle various GitHub URL formats
    const patterns = [
        /github\.com\/([^\/]+)\/([^\/\?#]+)/,  // https://github.com/owner/repo
        /github\.com:([^\/]+)\/([^\/\?#]+)/,   // git@github.com:owner/repo
    ];
    for (const pattern of patterns) {
        const match = url.match(pattern);
        if (match) {
            return { owner: match[1], repo: match[2].replace(/\.git$/, '') };
        }
    }
    return null;
}

// URL clone button handler
document.getElementById('repoUrlBtn').addEventListener('click', () => {
    const urlField = document.getElementById('repoUrlField');
    const url = urlField.value.trim();
    if (!url) return;

    const parsed = parseGitHubUrl(url);
    if (parsed) {
        fetchRepoCommits(parsed.owner, parsed.repo);
    } else {
        alert('Invalid GitHub URL. Please use format: https://github.com/owner/repo');
    }
});

// Allow Enter key in URL field
document.getElementById('repoUrlField').addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        document.getElementById('repoUrlBtn').click();
    }
});

// ============================================================================
// Repository Modal
// ============================================================================

let allRepos = [];

async function showRepoModal(showUserRepos = false) {
    const repoList = document.getElementById('repoList');
    const repoSearchSection = document.getElementById('repoSearchSection');
    const progress = document.getElementById('repoProgress');

    openModal('repoModal');
    progress.classList.add('hidden');

    if (!showUserRepos) {
        // Not logged in - just show URL input
        repoSearchSection.style.display = 'none';
        repoList.style.display = 'none';
        return;
    }

    // Logged in - show everything
    repoSearchSection.style.display = 'block';
    repoList.style.display = 'block';
    repoList.innerHTML = '<div class="loading-repos">Loading repositories...</div>';
    repoList.classList.remove('hidden');

    try {
        allRepos = [];
        let page = 1;
        let hasMore = true;

        while (hasMore) {
            const response = await fetch(`/api/repos?per_page=100&page=${page}&sort=updated`, {
                headers: { 'Authorization': `Bearer ${githubToken}` }
            });

            if (!response.ok) throw new Error('Failed to fetch repos');

            const repos = await response.json();
            allRepos = allRepos.concat(repos);

            hasMore = repos.length === 100;
            page++;
        }

        renderRepoList(allRepos);
    } catch (err) {
        console.error('Failed to fetch repos:', err);
        repoList.innerHTML = '<div class="error">Failed to load repositories</div>';
    }
}

function renderRepoList(repos) {
    const repoList = document.getElementById('repoList');
    const cachedRepos = getCachedReposList();
    const cachedMap = new Map(cachedRepos.map(r => [`${r.owner}/${r.repo}`, r]));

    if (repos.length === 0) {
        repoList.innerHTML = '<div class="no-repos">No repositories found</div>';
        return;
    }

    repoList.innerHTML = repos.map(repo => {
        const cacheKey = `${repo.owner.login}/${repo.name}`;
        const cached = cachedMap.get(cacheKey);
        return `
        <div class="repo-item ${cached ? 'repo-cached' : ''}" data-owner="${repo.owner.login}" data-repo="${repo.name}">
            <div class="repo-name">
                ${repo.private ? '<span class="repo-private">🔒</span>' : ''}
                ${repo.full_name}
                ${cached ? '<span class="repo-cached-badge" title="Cached">cached</span>' : ''}
                ${cached ? '<button class="repo-refresh-btn" title="Re-fetch from GitHub">↻</button>' : ''}
            </div>
            <div class="repo-meta">
                ${repo.language ? `<span class="repo-lang">${repo.language}</span>` : ''}
                <span class="repo-stars">⭐ ${repo.stargazers_count}</span>
                <span class="repo-updated">${formatDate(repo.updated_at)}</span>
                ${cached ? `<span class="repo-commits">${cached.commitCount} commits</span>` : ''}
            </div>
        </div>
    `}).join('');

    // Add click handlers
    repoList.querySelectorAll('.repo-item').forEach(item => {
        item.addEventListener('click', (e) => {
            // Check if refresh button was clicked
            if (e.target.classList.contains('repo-refresh-btn')) {
                e.stopPropagation();
                const owner = item.dataset.owner;
                const repo = item.dataset.repo;
                // Clear cache and re-fetch
                clearRepoCache(owner, repo);
                fetchRepoCommits(owner, repo);
                return;
            }
            const owner = item.dataset.owner;
            const repo = item.dataset.repo;
            fetchRepoCommits(owner, repo);
        });
    });
}

function formatDate(dateStr) {
    const date = new Date(dateStr);
    const now = new Date();
    const diff = now - date;
    const days = Math.floor(diff / (1000 * 60 * 60 * 24));

    if (days === 0) return 'today';
    if (days === 1) return 'yesterday';
    if (days < 30) return `${days} days ago`;
    if (days < 365) return `${Math.floor(days / 30)} months ago`;
    return `${Math.floor(days / 365)} years ago`;
}

// Search filter
document.getElementById('repoSearchInput').addEventListener('input', (e) => {
    const query = e.target.value.toLowerCase();
    const filtered = allRepos.filter(repo =>
        repo.full_name.toLowerCase().includes(query) ||
        (repo.description && repo.description.toLowerCase().includes(query))
    );
    renderRepoList(filtered);
});

// Close modal handlers
document.getElementById('repoModalClose').addEventListener('click', () => {
    closeModal('repoModal');
});

document.getElementById('repoModal').addEventListener('click', (e) => {
    if (e.target.id === 'repoModal') {
        closeModal('repoModal');
    }
});

// ============================================================================
// Repository Log Cache
// ============================================================================

const CACHE_KEY_PREFIX = 'gource_repo_';
const CACHE_VERSION = 1;

function getCacheKey(owner, repo) {
    return `${CACHE_KEY_PREFIX}${owner}_${repo}`;
}

function getCachedRepo(owner, repo) {
    try {
        const key = getCacheKey(owner, repo);
        const cached = localStorage.getItem(key);
        if (!cached) return null;

        const data = JSON.parse(cached);
        if (data.version !== CACHE_VERSION) return null;

        return data;
    } catch (err) {
        console.warn('Failed to read cache:', err);
        return null;
    }
}

function setCachedRepo(owner, repo, logLines, lastCommitSha, lastCommitDate) {
    try {
        const key = getCacheKey(owner, repo);
        const data = {
            version: CACHE_VERSION,
            owner,
            repo,
            logLines,
            lastCommitSha,
            lastCommitDate,
            cachedAt: Date.now()
        };
        localStorage.setItem(key, JSON.stringify(data));
    } catch (err) {
        console.warn('Failed to write cache:', err);
        // If storage is full, try to clear old caches
        if (err.name === 'QuotaExceededError') {
            clearOldCaches();
        }
    }
}

function clearOldCaches() {
    const keys = [];
    for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key.startsWith(CACHE_KEY_PREFIX)) {
            keys.push(key);
        }
    }
    // Remove oldest half
    if (keys.length > 0) {
        const toRemove = keys.slice(0, Math.ceil(keys.length / 2));
        toRemove.forEach(key => localStorage.removeItem(key));
    }
}

function clearRepoCache(owner, repo) {
    const key = getCacheKey(owner, repo);
    localStorage.removeItem(key);
}

function getCachedReposList() {
    const repos = [];
    for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key.startsWith(CACHE_KEY_PREFIX)) {
            try {
                const data = JSON.parse(localStorage.getItem(key));
                if (data.version === CACHE_VERSION) {
                    repos.push({
                        owner: data.owner,
                        repo: data.repo,
                        cachedAt: data.cachedAt,
                        commitCount: data.logLines.length
                    });
                }
            } catch (err) {}
        }
    }
    return repos.sort((a, b) => b.cachedAt - a.cachedAt);
}

// ============================================================================
// Rate Limit Tracking
// ============================================================================

let rateLimitRemaining = null;
let rateLimitTotal = null;
let rateLimitReset = null;

function updateRateLimitFromResponse(response) {
    const remaining = response.headers.get('X-RateLimit-Remaining');
    const limit = response.headers.get('X-RateLimit-Limit');
    const reset = response.headers.get('X-RateLimit-Reset');

    if (remaining) rateLimitRemaining = parseInt(remaining);
    if (limit) rateLimitTotal = parseInt(limit);
    if (reset) rateLimitReset = parseInt(reset);

    updateRateLimitDisplay();
}

function updateRateLimitDisplay() {
    let badge = document.getElementById('rateLimitBadge');
    if (!badge) {
        badge = document.createElement('div');
        badge.id = 'rateLimitBadge';
        badge.innerHTML = `
            <svg height="14" viewBox="0 0 16 16" width="14" fill="currentColor">
                <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z"/>
            </svg>
            <span class="rate-limit-text"></span>
            <span class="rate-limit-tooltip">GitHub API requests remaining this hour. Authenticated users get 5,000 requests/hour. Resets hourly.</span>
        `;
        document.body.appendChild(badge);
    }

    if (rateLimitRemaining !== null && rateLimitTotal !== null) {
        badge.querySelector('.rate-limit-text').textContent = `${rateLimitRemaining} / ${rateLimitTotal}`;
        badge.style.display = 'flex';

        // Color based on remaining
        const pct = rateLimitRemaining / rateLimitTotal;
        if (pct < 0.1) badge.className = 'rate-limit-critical';
        else if (pct < 0.3) badge.className = 'rate-limit-warning';
        else badge.className = '';
    }
}

// ============================================================================
// Git Clone & Log Generation (isomorphic-git with optimized tree diff)
// ============================================================================

// lightning-fs for isomorphic-git
let lfs = null;
let pfs = null;

// CORS proxy for git clone
const CORS_PROXY = 'https://cors.isomorphic-git.org';

function initLightningFs() {
    if (!lfs) {
        lfs = new LightningFS('gource-git', { wipe: true });
        pfs = lfs.promises;
    }
    return pfs;
}

// Fast tree diff using git.walk() with two TREE walkers
async function diffCommits(fs, dir, parentOid, commitOid) {
    const changes = [];

    if (!parentOid) {
        // First commit - all files are additions
        await git.walk({
            fs, dir,
            trees: [git.TREE({ ref: commitOid })],
            map: async (filepath, [entry]) => {
                if (filepath === '.' || !entry) return;
                if ((await entry.type()) === 'tree') return;
                changes.push({ path: filepath, action: 'A' });
            }
        });
    } else {
        // Compare two commits
        await git.walk({
            fs, dir,
            trees: [git.TREE({ ref: parentOid }), git.TREE({ ref: commitOid })],
            map: async (filepath, [parent, current]) => {
                if (filepath === '.') return;

                const parentType = parent ? await parent.type() : null;
                const currentType = current ? await current.type() : null;

                // Skip directories
                if (parentType === 'tree' || currentType === 'tree') return;

                const pOid = parent ? await parent.oid() : null;
                const cOid = current ? await current.oid() : null;

                if (!pOid && cOid) {
                    changes.push({ path: filepath, action: 'A' });
                } else if (pOid && !cOid) {
                    changes.push({ path: filepath, action: 'D' });
                } else if (pOid !== cOid) {
                    changes.push({ path: filepath, action: 'M' });
                }
            }
        });
    }

    return changes;
}

async function fetchRepoCommits(owner, repo) {
    const repoList = document.getElementById('repoList');
    const repoUrlInput = document.getElementById('repoUrlInput');
    const repoSearchSection = document.getElementById('repoSearchSection');
    const progress = document.getElementById('repoProgress');
    const progressText = progress.querySelector('.progress-text');
    const progressFill = progress.querySelector('.progress-fill');
    const progressDetail = progress.querySelector('.progress-detail');

    // Pause Gource to free up main thread
    if (gourcePause) gourcePause();

    // Hide everything except progress
    repoList.style.display = 'none';
    repoUrlInput.style.display = 'none';
    if (repoSearchSection) repoSearchSection.style.display = 'none';
    progress.classList.remove('hidden');
    progressFill.style.backgroundColor = '';

    const dir = `/${owner}_${repo}`;
    const url = `https://github.com/${owner}/${repo}`;

    try {
        progressText.textContent = 'Initializing...';
        progressFill.style.width = '5%';

        const pfs = initLightningFs();
        const fs = { promises: pfs };

        // Clean up
        try { await pfs.rmdir(dir, { recursive: true }); } catch(e) {}
        await pfs.mkdir(dir, { recursive: true });

        // Clone
        progressText.textContent = 'Cloning repository...';
        progressDetail.textContent = 'This may take a while for large repos';
        progressFill.style.width = '10%';

        await git.clone({
            fs, http: GitHttp, dir,
            corsProxy: CORS_PROXY,
            url,
            singleBranch: true,
            noCheckout: true,
            depth: 10000,
            onProgress: (event) => {
                if (event.phase === 'Receiving objects') {
                    const pct = Math.round((event.loaded / event.total) * 40) + 10;
                    progressFill.style.width = `${pct}%`;
                    progressDetail.textContent = `Receiving: ${Math.round(event.loaded / 1024)} KB`;
                } else if (event.phase === 'Resolving deltas') {
                    progressFill.style.width = '50%';
                    progressDetail.textContent = 'Resolving deltas...';
                }
            }
        });

        progressFill.style.width = '55%';
        progressText.textContent = 'Reading commits...';

        // Get commit history
        const commits = await git.log({ fs, dir, depth: 10000 });
        console.log(`Found ${commits.length} commits`);

        progressText.textContent = 'Processing file changes...';
        const logLines = [];
        const total = commits.length;

        // Process commits in reverse order (oldest first)
        for (let i = total - 1; i >= 0; i--) {
            const commit = commits[i];
            const timestamp = commit.commit.author.timestamp;
            const author = commit.commit.author.name || 'Unknown';
            const parentOid = commit.commit.parent[0] || null;

            // Use optimized tree diff
            const changes = await diffCommits(fs, dir, parentOid, commit.oid);

            for (const change of changes) {
                logLines.push(`${timestamp}|${author}|${change.action}|${change.path}`);
            }

            // Update progress
            const processed = total - i;
            if (processed % 10 === 0 || processed === total) {
                const pct = Math.round((processed / total) * 40) + 55;
                progressFill.style.width = `${pct}%`;
                progressDetail.textContent = `${processed} / ${total} commits`;
            }
        }

        progressFill.style.width = '100%';
        progressText.textContent = 'Loading visualization...';
        progressDetail.textContent = `${logLines.length} file changes found`;

        if (logLines.length === 0) {
            throw new Error('No file changes found');
        }

        setCachedRepo(owner, repo, logLines, 'isomorphic-git', new Date().toISOString());

        const logData = logLines.join('\n');

        if (gourceLoadLog) {
            gourceLoadLog(logData);
            closeModal('repoModal');
        }

        // Cleanup
        try { await pfs.rmdir(dir, { recursive: true }); } catch(e) {}

    } catch (err) {
        console.error('Failed to clone/process repository:', err);
        progressText.textContent = `Error: ${err.message}`;
        progressFill.style.width = '0%';
        progressFill.style.backgroundColor = '#f44';
        // Resume Gource if there was an error
        if (gourceResume) gourceResume();
    }
}

// ============================================================================
// Initialize
// ============================================================================

// Check for OAuth callback on page load
handleOAuthCallback();

// Fetch user if we have a token
if (githubToken) {
    fetchGitHubUser();
}

updateGitHubButton();

// Export for external use
window.GourceWeb = {
    loadLog: (data) => gourceLoadLog && gourceLoadLog(data),
    loadLogFromUrl: loadLogFromUrl,
    reset: () => gourceReset && gourceReset(),
    logout: logout
};
