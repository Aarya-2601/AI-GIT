require("dotenv").config();

const express = require("express");
const session = require("express-session");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

const app = express();

const PORT = process.env.PORT || 3000;

const PROFILE_STUDIO_URL =
    process.env.PROFILE_STUDIO_URL ||
    "https://techwithgen.github.io/profile-studio/";

const USERS_FILE =
    path.join(__dirname, "data", "users.json");

const GITHUB_API =
    "https://api.github.com";

const GITHUB_API_VERSION =
    "2026-03-10";

/* =========================================================
   BASIC SETUP
========================================================= */

app.use(express.json({ limit: "1mb" }));
app.use(express.urlencoded({ extended: true }));

app.use(
    session({
        secret:
            process.env.SESSION_SECRET ||
            "development-secret-change-me",

        resave: false,

        saveUninitialized: false,

        cookie: {
            httpOnly: true,
            sameSite: "lax",
            secure: false
        }
    })
);

app.use(express.static(
    path.join(__dirname, "public")
));

/* =========================================================
   USER STORAGE
========================================================= */

function ensureUserFile() {

    const directory =
        path.dirname(USERS_FILE);

    if(!fs.existsSync(directory)) {
        fs.mkdirSync(
            directory,
            { recursive: true }
        );
    }

    if(!fs.existsSync(USERS_FILE)) {

        fs.writeFileSync(
            USERS_FILE,
            "{}",
            "utf8"
        );
    }
}

function readUsers() {

    ensureUserFile();

    try {

        return JSON.parse(
            fs.readFileSync(
                USERS_FILE,
                "utf8"
            )
        );

    } catch(error) {

        console.error(
            "Could not read users:",
            error
        );

        return {};
    }
}

function writeUsers(users) {

    ensureUserFile();

    fs.writeFileSync(
        USERS_FILE,
        JSON.stringify(
            users,
            null,
            2
        ),
        "utf8"
    );
}

/* =========================================================
   GITHUB API HELPER
========================================================= */

async function githubRequest(
    token,
    endpoint,
    options = {}
) {

    const response =
        await fetch(
            `${GITHUB_API}${endpoint}`,
            {
                ...options,

                headers: {
                    "Accept":
                        "application/vnd.github+json",

                    "Authorization":
                        `Bearer ${token}`,

                    "X-GitHub-Api-Version":
                        GITHUB_API_VERSION,

                    "Content-Type":
                        "application/json",

                    ...(options.headers || {})
                }
            }
        );

    const text =
        await response.text();

    let data;

    try {
        data = JSON.parse(text);
    } catch {
        data = {
            message: text
        };
    }

    if(!response.ok) {

        const error =
            new Error(
                data.message ||
                "GitHub API request failed"
            );

        error.status =
            response.status;

        error.github =
            data;

        throw error;
    }

    return data;
}

/* =========================================================
   GITHUB LOGIN
========================================================= */

app.get(
    "/auth/github",
    (req, res) => {

        if(
            !process.env.GITHUB_CLIENT_ID ||
            !process.env.GITHUB_CLIENT_SECRET
        ) {

            return res
                .status(500)
                .send(
                    "GitHub OAuth is not configured."
                );
        }

        const state =
            crypto
                .randomBytes(24)
                .toString("hex");

        req.session.githubOAuthState =
            state;

        const params =
            new URLSearchParams({

                client_id:
                    process.env.GITHUB_CLIENT_ID,

                redirect_uri:
                    process.env.GITHUB_CALLBACK_URL,

                scope:
                    "read:user user:email public_repo",

                state

            });

        const url =
            `https://github.com/login/oauth/authorize?${params}`;

        res.redirect(url);
    }
);

/* =========================================================
   GITHUB CALLBACK
========================================================= */

app.get(
    "/auth/github/callback",
    async (req, res) => {

        const {
            code,
            state,
            error
        } = req.query;

        if(error) {

            return res
                .status(400)
                .send(
                    `GitHub authorization failed: ${error}`
                );
        }

        if(
            !state ||
            state !==
            req.session.githubOAuthState
        ) {

            return res
                .status(403)
                .send(
                    "Invalid OAuth state."
                );
        }

        delete req.session.githubOAuthState;

        if(!code) {

            return res
                .status(400)
                .send(
                    "Missing GitHub authorization code."
                );
        }

        try {

            /* Exchange code for token */

            const tokenResponse =
                await fetch(
                    "https://github.com/login/oauth/access_token",
                    {
                        method: "POST",

                        headers: {
                            "Accept":
                                "application/json",

                            "Content-Type":
                                "application/json"
                        },

                        body: JSON.stringify({

                            client_id:
                                process.env.GITHUB_CLIENT_ID,

                            client_secret:
                                process.env.GITHUB_CLIENT_SECRET,

                            code,

                            redirect_uri:
                                process.env.GITHUB_CALLBACK_URL

                        })
                    }
                );

            const tokenData =
                await tokenResponse.json();

            if(
                !tokenData.access_token
            ) {

                throw new Error(
                    tokenData.error_description ||
                    "Could not obtain GitHub access token."
                );
            }

            const token =
                tokenData.access_token;

            /* Get authenticated GitHub user */

            const githubUser =
                await githubRequest(
                    token,
                    "/user"
                );

            if(!githubUser.login) {

                throw new Error(
                    "GitHub did not return a username."
                );
            }

            /*
               Store token ONLY server-side.
               Never send it to frontend.
            */

            req.session.user = {
                githubId:
                    githubUser.id,

                username:
                    githubUser.login,

                name:
                    githubUser.name ||
                    githubUser.login,

                avatar:
                    githubUser.avatar_url,

                token
            };

            /* Save/update local user record */

            const users =
                readUsers();

            const existing =
                users[
                    githubUser.login
                ] || {};

            users[
                githubUser.login
            ] = {

                ...existing,

                githubId:
                    githubUser.id,

                username:
                    githubUser.login,

                githubName:
                    githubUser.name,

                avatar:
                    githubUser.avatar_url,

                updatedAt:
                    new Date().toISOString()
            };

            writeUsers(users);

            /*
               If profile already exists,
               go straight to overview.
               Otherwise onboarding.
            */

            if(existing.profileCompleted) {

                return res.redirect(
                    `/overview.html?username=${encodeURIComponent(
                        githubUser.login
                    )}`
                );
            }

            res.redirect(
                `/onboarding.html?username=${encodeURIComponent(
                    githubUser.login
                )}`
            );

        } catch(error) {

            console.error(
                "GitHub OAuth error:",
                error
            );

            res
                .status(500)
                .send(
                    "Could not complete GitHub login."
                );
        }
    }
);

/* =========================================================
   AUTH MIDDLEWARE
========================================================= */

function requireAuth(
    req,
    res,
    next
) {

    if(
        !req.session.user ||
        !req.session.user.token
    ) {

        return res
            .status(401)
            .json({
                error:
                    "Not authenticated"
            });
    }

    next();
}

/* =========================================================
   CURRENT USER
========================================================= */

app.get(
    "/api/me",
    requireAuth,
    (req, res) => {

        const {
            token,
            ...safeUser
        } = req.session.user;

        res.json({
            user: safeUser
        });
    }
);

/* =========================================================
   PROFILE
========================================================= */

app.get(
    "/api/profile",
    requireAuth,
    (req, res) => {

        const username =
            req.session.user.username;

        const users =
            readUsers();

        const user =
            users[username];

        res.json({
            profile:
                user?.profile ||
                null
        });
    }
);

/* =========================================================
   README GENERATOR
========================================================= */

function clean(value) {

    return String(
        value || ""
    ).trim();
}

function makeREADME(profile) {

    const lines = [];

    lines.push(
        `# Hi there! I'm ${clean(profile.displayName)}`
    );

    lines.push("");

    if(profile.headline1) {

        lines.push(
            `> ${clean(profile.headline1)}`
        );

        lines.push("");
    }

    if(profile.headline2) {

        lines.push(
            `> ${clean(profile.headline2)}`
        );

        lines.push("");
    }

    lines.push(
        "## 🚀 About Me"
    );

    lines.push("");

    if(profile.bio) {

        lines.push(
            clean(profile.bio)
        );

        lines.push("");
    }

    function add(
        emoji,
        label,
        value
    ) {

        if(value) {

            lines.push(
                `${emoji} **${label}:** ${clean(value)}`
            );
        }
    }

    add(
        "🔭",
        "Currently working on",
        profile.workingOn
    );

    add(
        "🌱",
        "Currently learning",
        profile.learning
    );

    add(
        "👯",
        "Looking to collaborate on",
        profile.collaborate
    );

    add(
        "🤔",
        "Looking for help with",
        profile.help
    );

    add(
        "💬",
        "Ask me about",
        profile.askMe
    );

    add(
        "😄",
        "Pronouns",
        profile.pronouns
    );

    add(
        "⚡",
        "Fun fact",
        profile.funFact
    );

    lines.push("");

    lines.push(
        "## 🛠️ Tech Stack"
    );

    lines.push("");

    if(
        Array.isArray(profile.techStack) &&
        profile.techStack.length
    ) {

        lines.push(
            profile.techStack
                .map(
                    item =>
                        `\`${clean(item)}\``
                )
                .join(" ")
        );
    }

    lines.push("");

    lines.push(
        "## 🔗 Connect"
    );

    lines.push("");

    if(profile.portfolio) {

        lines.push(
            `[Portfolio](${profile.portfolio})`
        );
    }

    if(profile.linkedin) {

        lines.push(
            `[LinkedIn](${profile.linkedin})`
        );
    }

    if(profile.instagram) {

        lines.push(
            `[Instagram](${profile.instagram})`
        );
    }

    if(profile.email) {

        lines.push(
            `[Email](mailto:${profile.email})`
        );
    }

    lines.push("");

    lines.push("---");

    lines.push("");

    lines.push(
        `Profile generated with GitHub Profile Studio.`
    );

    return lines.join("\n");
}

/* =========================================================
   CREATE / UPDATE GITHUB PROFILE REPOSITORY
========================================================= */

async function createOrUpdateProfileRepo(
    token,
    username,
    readme
) {

    let repository;

    /*
       First check whether username/username exists.
    */

    try {

        repository =
            await githubRequest(
                token,
                `/repos/${encodeURIComponent(
                    username
                )}/${encodeURIComponent(
                    username
                )}`
            );

    } catch(error) {

        if(error.status !== 404) {
            throw error;
        }

        /*
           Repository does not exist.
           Create it.
        */

        repository =
            await githubRequest(
                token,
                "/user/repos",
                {
                    method: "POST",

                    body:
                        JSON.stringify({

                            name:
                                username,

                            description:
                                `${username}'s GitHub profile`,

                            private:
                                false,

                            has_issues:
                                false,

                            has_projects:
                                false,

                            has_wiki:
                                false,

                            has_discussions:
                                false
                        })
                }
            );
    }

    /*
       Check whether README.md already exists.
    */

    let existingFile = null;

    try {

        existingFile =
            await githubRequest(
                token,
                `/repos/${encodeURIComponent(
                    username
                )}/${encodeURIComponent(
                    username
                )}/contents/README.md`
            );

    } catch(error) {

        if(error.status !== 404) {
            throw error;
        }
    }

    /*
       GitHub Contents API expects Base64.
    */

    const content =
        Buffer
            .from(
                readme,
                "utf8"
            )
            .toString("base64");

    const body = {

        message:
            existingFile
                ? "Update profile README"
                : "Create profile README",

        content

    };

    if(existingFile?.sha) {

        body.sha =
            existingFile.sha;
    }

    await githubRequest(
        token,
        `/repos/${encodeURIComponent(
            username
        )}/${encodeURIComponent(
            username
        )}/contents/README.md`,
        {
            method: "PUT",

            body:
                JSON.stringify(body)
        }
    );

    return {
        repoUrl:
            repository.html_url ||
            `https://github.com/${username}/${username}`,

        repoName:
            username,

        readmeUpdated:
            true
    };
}

/* =========================================================
   SAVE PROFILE + CREATE REPO + README
========================================================= */

app.post(
    "/api/profile/setup",
    requireAuth,
    async (req, res) => {

        try {

            const username =
                req.session.user.username;

            const profile =
                req.body.profile;

            if(!profile) {

                return res
                    .status(400)
                    .json({
                        message:
                            "Profile data is required."
                    });
            }

            /*
               SECURITY:
               Never let frontend decide which
               GitHub account receives the repo.
            */

            profile.username =
                username;

            profile.displayName =
                clean(
                    profile.displayName
                ) ||
                req.session.user.name ||
                username;

            profile.avatar =
                profile.avatar ||
                req.session.user.avatar;

            const readme =
                makeREADME(profile);

            const githubResult =
                await createOrUpdateProfileRepo(
                    req.session.user.token,
                    username,
                    readme
                );

            const users =
                readUsers();

            users[username] = {

                ...(users[username] || {}),

                username,

                githubId:
                    req.session.user.githubId,

                avatar:
                    profile.avatar,

                profile,

                profileCompleted:
                    true,

                repoUrl:
                    githubResult.repoUrl,

                updatedAt:
                    new Date().toISOString()
            };

            writeUsers(users);

            res.json({

                success:
                    true,

                username,

                repoUrl:
                    githubResult.repoUrl,

                profile,

                profileStudioUrl:
                    `${PROFILE_STUDIO_URL}?username=${encodeURIComponent(
                        username
                    )}`,

                overviewUrl:
                    `/overview.html?username=${encodeURIComponent(
                        username
                    )}`
            });

        } catch(error) {

            console.error(
                "Profile setup error:",
                error
            );

            res
                .status(
                    error.status || 500
                )
                .json({

                    message:
                        error.message ||
                        "Could not create GitHub profile."
                });
        }
    }
);

/* =========================================================
   UPDATE PROFILE
========================================================= */

app.put(
    "/api/profile",
    requireAuth,
    async (req, res) => {

        try {

            const username =
                req.session.user.username;

            const profile =
                req.body.profile;

            if(!profile) {

                return res
                    .status(400)
                    .json({
                        message:
                            "Profile data is required."
                    });
            }

            profile.username =
                username;

            const readme =
                makeREADME(profile);

            const githubResult =
                await createOrUpdateProfileRepo(
                    req.session.user.token,
                    username,
                    readme
                );

            const users =
                readUsers();

            users[username] = {

                ...(users[username] || {}),

                profile,

                profileCompleted:
                    true,

                repoUrl:
                    githubResult.repoUrl,

                updatedAt:
                    new Date().toISOString()
            };

            writeUsers(users);

            res.json({

                success:
                    true,

                profile,

                repoUrl:
                    githubResult.repoUrl
            });

        } catch(error) {

            console.error(
                "Profile update error:",
                error
            );

            res
                .status(
                    error.status || 500
                )
                .json({

                    message:
                        error.message ||
                        "Could not update profile."
                });
        }
    }
);

/* =========================================================
   LOGOUT
========================================================= */

app.post(
    "/api/logout",
    (req, res) => {

        req.session.destroy(
            () => {

                res.json({
                    success:
                        true
                });
            }
        );
    }
);

/* =========================================================
   START SERVER
========================================================= */

app.listen(
    PORT,
    () => {

        console.log(
            `Server running at http://localhost:${PORT}`
        );
    }
);