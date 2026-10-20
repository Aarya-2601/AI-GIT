const {Pool}=require('pg');
require('dotenv').config();

const pool=new Pool({
    host: process.env.PG_HOST || 'localhost',
    port: parseInt(process.env.PG_PORT, 10) || 5432,
    user: process.env.PG_USER || 'postgres',
    password: process.env.PG_PASSWORD || 'postgres',
    database: process.env.PG_DATABASE || 'aigit_db',
});

pool.on('connect', ()=>{
    console.log('PostgreSQL database connected.');
});

pool.on('error', (err)=>{
    console.error('Unexpected PostgreSQL client error:', err);
});

module.exports={
    query: (text, params) => pool.query(text, params),
    pool,
};