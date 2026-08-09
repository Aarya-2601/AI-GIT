const express=require('express');
const router=express.Router();
const {try_pull}=require('../controllers/pull.controller.js')

router.post('/urls', try_pull);

module.exports={
    router
};