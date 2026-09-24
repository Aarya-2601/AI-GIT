const fs = require('fs');
const path = require('path');

const REPOS_DIR = path.join(
    __dirname,
    '..',
    '..',
    'data',
    'repos'
);


const ensureRepoDirectory = () => {
    fs.mkdirSync(REPOS_DIR, {
        recursive: true
    });
};


const validateRepoName = (repoName) => {
    if (
        !repoName ||
        typeof repoName !== 'string' ||
        !/^[A-Za-z0-9._-]+$/.test(repoName)
    ) {
        throw new Error('Invalid repository name');
    }
};


const getRepoPath = (repoName) => {
    validateRepoName(repoName);

    return path.join(
        REPOS_DIR,
        `${repoName}.json`
    );
};


const saveRepository = async (
    repoName,
    head,
    refs,
    objects
) => {
    ensureRepoDirectory();

    const repoPath =
        getRepoPath(repoName);


    const repository = {
        name: repoName,
        head: head,
        refs: refs,
        objects: objects,
        updated_at: new Date().toISOString()
    };


    fs.writeFileSync(
        repoPath,
        JSON.stringify(repository, null, 2),
        'utf8'
    );


    return repository;
};


const getRepository = async (repoName) => {
    ensureRepoDirectory();

    const repoPath =
        getRepoPath(repoName);


    if (!fs.existsSync(repoPath)) {
        return null;
    }


    const contents =
        fs.readFileSync(
            repoPath,
            'utf8'
        );


    return JSON.parse(contents);
};


module.exports = {
    saveRepository,
    getRepository
};