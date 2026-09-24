const express = require('express');

const router = express.Router();

const {
    try_clone
} = require('../controllers/clone.controller.js');


router.get(
    '/:repoName',
    try_clone
);


module.exports = {
    router
};