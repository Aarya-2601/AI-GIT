const express = require('express');

const router = express.Router();

const {try_push,finalize_push} = require('../controllers/push.controller.js');

router.post('/negotiate',try_push);

router.post('/finalize',finalize_push);

module.exports = {router};