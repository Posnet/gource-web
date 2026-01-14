// Gource Web - JavaScript Interface

let gourceModule = null;
let gourceLoadLog = null;

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
        text.textContent = githubUser.login;
        btn.classList.add('logged-in');
    } else if (githubToken) {
        text.textContent = 'Loading...';
    } else {
        text.textContent = 'Login with GitHub';
        btn.classList.remove('logged-in');
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
    const modal = document.getElementById('repoModal');
    const repoList = document.getElementById('repoList');
    const progress = document.getElementById('repoProgress');

    modal.classList.add('show');
    repoList.innerHTML = '<div class="loading-repos">Loading repositories...</div>';
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
    document.getElementById('repoModal').classList.remove('show');
});

document.getElementById('repoModal').addEventListener('click', (e) => {
    if (e.target.id === 'repoModal') {
        document.getElementById('repoModal').classList.remove('show');
    }
});

// ============================================================================
// Fetch Commits & Generate Gource Log
// ============================================================================

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

            if (!response.ok) {
                if (response.status === 409) {
                    // Empty repository
                    throw new Error('Repository is empty');
                }
                throw new Error('Failed to fetch commits');
            }

            const pageCommits = await response.json();
            commits = commits.concat(pageCommits);

            progressDetail.textContent = `${commits.length} commits found...`;

            // Check Link header for pagination
            const linkHeader = response.headers.get('Link');
            hasMore = linkHeader && linkHeader.includes('rel="next"');
            page++;

            // Safety limit
            if (commits.length >= 5000) {
                console.warn('Limiting to 5000 commits');
                hasMore = false;
            }
        }

        if (commits.length === 0) {
            throw new Error('No commits found');
        }

        // Step 2: Fetch file details for each commit
        progressText.textContent = 'Fetching file changes...';
        const gourceLog = [];

        for (let i = 0; i < commits.length; i++) {
            const commit = commits[i];
            const percent = Math.round((i / commits.length) * 100);
            progressFill.style.width = `${percent}%`;
            progressDetail.textContent = `${i + 1} / ${commits.length} commits`;

            try {
                const response = await fetch(`/api/repos/${owner}/${repo}/commits/${commit.sha}`, {
                    headers: { 'Authorization': `Bearer ${githubToken}` }
                });

                if (response.ok) {
                    const detail = await response.json();
                    const timestamp = Math.floor(new Date(commit.commit.author.date).getTime() / 1000);
                    const author = commit.commit.author.name || commit.author?.login || 'Unknown';

                    if (detail.files) {
                        for (const file of detail.files) {
                            let action = 'M'; // Modified
                            if (file.status === 'added') action = 'A';
                            else if (file.status === 'removed') action = 'D';
                            else if (file.status === 'renamed') action = 'M';

                            gourceLog.push(`${timestamp}|${author}|${action}|${file.filename}`);
                        }
                    }
                }

                // Rate limit: small delay between requests
                if (i % 10 === 0) {
                    await new Promise(r => setTimeout(r, 100));
                }
            } catch (err) {
                console.warn(`Failed to fetch commit ${commit.sha}:`, err);
            }
        }

        progressFill.style.width = '100%';
        progressText.textContent = 'Loading visualization...';

        // Sort by timestamp and load
        gourceLog.sort((a, b) => {
            const ta = parseInt(a.split('|')[0]);
            const tb = parseInt(b.split('|')[0]);
            return ta - tb;
        });

        const logData = gourceLog.join('\n');

        if (gourceLoadLog) {
            gourceLoadLog(logData);
            document.getElementById('repoModal').classList.remove('show');
            canvas.focus();
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
    logout: logout
};
