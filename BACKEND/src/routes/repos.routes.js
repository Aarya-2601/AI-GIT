const express = require('express');

const router = express.Router();

const {
    get_repo_meta
} = require('../controllers/repos.controller.js');


router.get('/:repoName', get_repo_meta);


module.exports = { router };
