// Gource Web - JavaScript Interface

let gourceModule = null;
let gourceLoadLog = null;
let gourceReset = null;

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
        // Already logged in - show repo selector
        showRepoModal();
    } else {
        // Start OAuth flow
        try {
            const response = await fetch('/api/auth/github');
            const data = await response.json();
            if (data.url) {
                window.location.href = data.url;
            }
        } catch (err) {
            console.error('Failed to start OAuth:', err);
        }
    }
});

// ============================================================================
// Repository Modal
// ============================================================================

let allRepos = [];

async function showRepoModal() {
    const repoList = document.getElementById('repoList');
    const progress = document.getElementById('repoProgress');

    openModal('repoModal');
    repoList.innerHTML = '<div class="loading-repos">Loading repositories...</div>';
    repoList.classList.remove('hidden');
    progress.classList.add('hidden');

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

    if (repos.length === 0) {
        repoList.innerHTML = '<div class="no-repos">No repositories found</div>';
        return;
    }

    repoList.innerHTML = repos.map(repo => `
        <div class="repo-item" data-owner="${repo.owner.login}" data-repo="${repo.name}">
            <div class="repo-name">
                ${repo.private ? '<span class="repo-private">🔒</span>' : ''}
                ${repo.full_name}
            </div>
            <div class="repo-meta">
                ${repo.language ? `<span class="repo-lang">${repo.language}</span>` : ''}
                <span class="repo-stars">⭐ ${repo.stargazers_count}</span>
                <span class="repo-updated">${formatDate(repo.updated_at)}</span>
            </div>
        </div>
    `).join('');

    // Add click handlers
    repoList.querySelectorAll('.repo-item').forEach(item => {
        item.addEventListener('click', () => {
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
// Fetch Commits & Generate Gource Log
// ============================================================================

// Concurrent fetch with limit
async function fetchConcurrent(urls, concurrency, fetchFn) {
    const results = [];
    let index = 0;

    async function worker() {
        while (index < urls.length) {
            const i = index++;
            try {
                results[i] = await fetchFn(urls[i], i);
            } catch (err) {
                results[i] = { error: err };
            }
        }
    }

    const workers = Array(Math.min(concurrency, urls.length)).fill(null).map(() => worker());
    await Promise.all(workers);
    return results;
}

async function fetchRepoCommits(owner, repo) {
    const repoList = document.getElementById('repoList');
    const progress = document.getElementById('repoProgress');
    const progressText = progress.querySelector('.progress-text');
    const progressFill = progress.querySelector('.progress-fill');
    const progressDetail = progress.querySelector('.progress-detail');

    repoList.classList.add('hidden');
    progress.classList.remove('hidden');

    try {
        // Step 1: Fetch all commits (paginated)
        progressText.textContent = 'Fetching commit list...';
        progressFill.style.width = '0%';

        let commits = [];
        let page = 1;
        let hasMore = true;

        while (hasMore) {
            const response = await fetch(`/api/repos/${owner}/${repo}/commits?per_page=100&page=${page}`, {
                headers: { 'Authorization': `Bearer ${githubToken}` }
            });

            updateRateLimitFromResponse(response);

            if (!response.ok) {
                if (response.status === 409) {
                    throw new Error('Repository is empty');
                }
                throw new Error('Failed to fetch commits');
            }

            const pageCommits = await response.json();
            commits = commits.concat(pageCommits);

            progressDetail.textContent = `${commits.length} commits found...`;

            const linkHeader = response.headers.get('Link');
            hasMore = linkHeader && linkHeader.includes('rel="next"');
            page++;

            if (commits.length >= 5000) {
                console.warn('Limiting to 5000 commits');
                hasMore = false;
            }
        }

        if (commits.length === 0) {
            throw new Error('No commits found');
        }

        // Step 2: Fetch file details concurrently (10 at a time)
        progressText.textContent = 'Fetching file changes...';
        const gourceLog = [];
        let completed = 0;

        const commitDetails = await fetchConcurrent(
            commits,
            10, // concurrency limit
            async (commit, idx) => {
                const response = await fetch(`/api/repos/${owner}/${repo}/commits/${commit.sha}`, {
                    headers: { 'Authorization': `Bearer ${githubToken}` }
                });

                updateRateLimitFromResponse(response);

                completed++;
                const percent = Math.round((completed / commits.length) * 100);
                progressFill.style.width = `${percent}%`;
                progressDetail.textContent = `${completed} / ${commits.length} commits`;

                if (response.ok) {
                    return { commit, detail: await response.json() };
                }
                return { commit, detail: null };
            }
        );

        // Process results
        for (const result of commitDetails) {
            if (result.error || !result.detail) continue;

            const { commit, detail } = result;
            const timestamp = Math.floor(new Date(commit.commit.author.date).getTime() / 1000);
            const author = commit.commit.author.name || commit.author?.login || 'Unknown';

            if (detail.files) {
                for (const file of detail.files) {
                    let action = 'M';
                    if (file.status === 'added') action = 'A';
                    else if (file.status === 'removed') action = 'D';
                    else if (file.status === 'renamed') action = 'M';

                    gourceLog.push(`${timestamp}|${author}|${action}|${file.filename}`);
                }
            }
        }

        progressFill.style.width = '100%';
        progressText.textContent = 'Loading visualization...';

        // Sort by timestamp
        gourceLog.sort((a, b) => {
            const ta = parseInt(a.split('|')[0]);
            const tb = parseInt(b.split('|')[0]);
            return ta - tb;
        });

        const logData = gourceLog.join('\n');

        if (gourceLoadLog) {
            gourceLoadLog(logData);
            closeModal('repoModal');
        }

    } catch (err) {
        console.error('Failed to fetch commits:', err);
        progressText.textContent = `Error: ${err.message}`;
        progressFill.style.width = '0%';
        progressFill.style.backgroundColor = '#f44';
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
