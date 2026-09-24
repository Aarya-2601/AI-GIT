const fs = require('fs');
const path = require('path');

const REPOS_DIR = path.join(
    __dirname,
    '..',
    '..',
    'data',
    'repos'
);


// Make sure the repository metadata directory exists.
const ensureRepoDirectory = () => {
    fs.mkdirSync(REPOS_DIR, {
        recursive: true
    });
};


// Prevent repository names from escaping BACKEND/data/repos.
const validateRepoName = (repoName) => {
    if (
        !repoName ||
        typeof repoName !== 'string' ||
        !/^[A-Za-z0-9._-]+$/.test(repoName)
    ) {
        throw new Error('Invalid repository name');
    }
};


// Return the metadata file used for one remote repository.
const getRepoPath = (repoName) => {
    validateRepoName(repoName);

    return path.join(
        REPOS_DIR,
        `${repoName}.json`
    );
};


// Save the authoritative remote state of a repository.
const saveRepository = async (
    repoName,
    head,
    refs
) => {
    ensureRepoDirectory();

    const repoPath = getRepoPath(repoName);

    const repository = {
        name: repoName,
        head: head,
        refs: refs,
        updated_at: new Date().toISOString()
    };

    fs.writeFileSync(
        repoPath,
        JSON.stringify(repository, null, 2),
        'utf8'
    );

    return repository;
};


// Load repository metadata.
// Returns null when the requested repository does not exist.
const getRepository = async (repoName) => {
    ensureRepoDirectory();

    const repoPath = getRepoPath(repoName);

    if (!fs.existsSync(repoPath)) {
        return null;
    }

    const contents = fs.readFileSync(
        repoPath,
        'utf8'
    );

    return JSON.parse(contents);
};


module.exports = {
    saveRepository,
    getRepository
};