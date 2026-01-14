// Gource Web - Cloudflare Worker
// Handles GitHub OAuth and proxies API requests

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);

    // Handle API routes
    if (url.pathname.startsWith('/api/')) {
      return handleAPI(request, env, url);
    }

    // Static assets handled by Cloudflare Assets
    return env.ASSETS.fetch(request);
  }
};

async function handleAPI(request, env, url) {
  const corsHeaders = {
    'Access-Control-Allow-Origin': url.origin,
    'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type, Authorization',
  };

  if (request.method === 'OPTIONS') {
    return new Response(null, { headers: corsHeaders });
  }

  try {
    // OAuth: Start GitHub authorization
    if (url.pathname === '/api/auth/github') {
      const state = crypto.randomUUID();
      const githubAuthUrl = new URL('https://github.com/login/oauth/authorize');
      githubAuthUrl.searchParams.set('client_id', env.GITHUB_CLIENT_ID);
      githubAuthUrl.searchParams.set('redirect_uri', `${url.origin}/api/auth/github/callback`);
      githubAuthUrl.searchParams.set('scope', 'repo');
      githubAuthUrl.searchParams.set('state', state);

      return new Response(JSON.stringify({ url: githubAuthUrl.toString(), state }), {
        headers: { ...corsHeaders, 'Content-Type': 'application/json' }
      });
    }

    // OAuth: Exchange code for token
    if (url.pathname === '/api/auth/github/callback') {
      const code = url.searchParams.get('code');
      const state = url.searchParams.get('state');

      if (!code) {
        return new Response(JSON.stringify({ error: 'Missing code parameter' }), {
          status: 400,
          headers: { ...corsHeaders, 'Content-Type': 'application/json' }
        });
      }

      const tokenResponse = await fetch('https://github.com/login/oauth/access_token', {
        method: 'POST',
        headers: {
          'Accept': 'application/json',
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          client_id: env.GITHUB_CLIENT_ID,
          client_secret: env.GITHUB_CLIENT_SECRET,
          code: code,
        }),
      });

      const tokenData = await tokenResponse.json();

      if (tokenData.error) {
        return new Response(JSON.stringify({ error: tokenData.error_description || tokenData.error }), {
          status: 400,
          headers: { ...corsHeaders, 'Content-Type': 'application/json' }
        });
      }

      // Redirect back to app with token in hash (client-side only)
      const redirectUrl = new URL('/', url.origin);
      redirectUrl.hash = `access_token=${tokenData.access_token}&token_type=${tokenData.token_type}&scope=${tokenData.scope}`;

      return Response.redirect(redirectUrl.toString(), 302);
    }

    // Get current user
    if (url.pathname === '/api/user') {
      return proxyGitHubAPI(request, 'https://api.github.com/user', corsHeaders);
    }

    // List user repos
    if (url.pathname === '/api/repos') {
      const perPage = url.searchParams.get('per_page') || '100';
      const page = url.searchParams.get('page') || '1';
      const sort = url.searchParams.get('sort') || 'updated';
      return proxyGitHubAPI(
        request,
        `https://api.github.com/user/repos?per_page=${perPage}&page=${page}&sort=${sort}`,
        corsHeaders
      );
    }

    // Get repo commits
    const commitsMatch = url.pathname.match(/^\/api\/repos\/([^/]+)\/([^/]+)\/commits$/);
    if (commitsMatch) {
      const [, owner, repo] = commitsMatch;
      const perPage = url.searchParams.get('per_page') || '100';
      const page = url.searchParams.get('page') || '1';
      const sha = url.searchParams.get('sha') || '';
      let apiUrl = `https://api.github.com/repos/${owner}/${repo}/commits?per_page=${perPage}&page=${page}`;
      if (sha) apiUrl += `&sha=${sha}`;
      return proxyGitHubAPI(request, apiUrl, corsHeaders);
    }

    // Get single commit (with file details)
    const commitMatch = url.pathname.match(/^\/api\/repos\/([^/]+)\/([^/]+)\/commits\/([^/]+)$/);
    if (commitMatch) {
      const [, owner, repo, sha] = commitMatch;
      return proxyGitHubAPI(
        request,
        `https://api.github.com/repos/${owner}/${repo}/commits/${sha}`,
        corsHeaders
      );
    }

    // Get repo branches
    const branchesMatch = url.pathname.match(/^\/api\/repos\/([^/]+)\/([^/]+)\/branches$/);
    if (branchesMatch) {
      const [, owner, repo] = branchesMatch;
      return proxyGitHubAPI(
        request,
        `https://api.github.com/repos/${owner}/${repo}/branches`,
        corsHeaders
      );
    }

    return new Response(JSON.stringify({ error: 'Not found' }), {
      status: 404,
      headers: { ...corsHeaders, 'Content-Type': 'application/json' }
    });

  } catch (error) {
    return new Response(JSON.stringify({ error: error.message }), {
      status: 500,
      headers: { ...corsHeaders, 'Content-Type': 'application/json' }
    });
  }
}

async function proxyGitHubAPI(request, apiUrl, corsHeaders) {
  const authHeader = request.headers.get('Authorization');

  if (!authHeader) {
    return new Response(JSON.stringify({ error: 'Authorization header required' }), {
      status: 401,
      headers: { ...corsHeaders, 'Content-Type': 'application/json' }
    });
  }

  const response = await fetch(apiUrl, {
    headers: {
      'Authorization': authHeader,
      'Accept': 'application/vnd.github.v3+json',
      'User-Agent': 'Gource-Web',
    },
  });

  const data = await response.json();

  // Forward rate limit headers
  const headers = {
    ...corsHeaders,
    'Content-Type': 'application/json',
  };

  const rateLimitHeaders = ['X-RateLimit-Limit', 'X-RateLimit-Remaining', 'X-RateLimit-Reset', 'Link'];
  for (const header of rateLimitHeaders) {
    const value = response.headers.get(header);
    if (value) headers[header] = value;
  }

  return new Response(JSON.stringify(data), {
    status: response.status,
    headers,
  });
}
