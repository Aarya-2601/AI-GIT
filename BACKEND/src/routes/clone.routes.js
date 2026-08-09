const express=require('express')
const router=express.Router();
const{try_clone}=require('../controllers/clone.controller');

router.get('clone/:repoName', try_clone);

module.exports={
    router
};