const express = require('express');
const cors = require('cors');

require('dotenv').config();

const {
    bucket_exists
} = require('./services/minioservice.js');

const pushRoutes =
    require('./routes/push.routes.js').router;

const cloneRoutes =
    require('./routes/clone.routes.js').router;

const pullRoutes =
    require('./routes/pull.routes.js').router;

const reposRoutes =
    require('./routes/repos.routes.js').router;


const app = express();

const PORT =
    process.env.PORT || 3000;


app.use(cors());
app.use(express.json());


app.get('/health', (req, res) => {

    res.status(200).send({
        status: 'ok',
        service: 'ai-git backend'
    });

});


app.use(
    '/api/v1/push',
    pushRoutes
);


app.use(
    '/api/v1/clone',
    cloneRoutes
);

app.use(
    '/api/v1/pull',
    pullRoutes
);

app.use(
    '/api/v1/repos',
    reposRoutes
);


app.listen(PORT, async () => {

    console.log(
        `AI-Git backend live on http://localhost:${PORT}`
    );

    try {

        await bucket_exists();

    }
    catch (err) {

        console.error(
            'Failed to initialize MinIO bucket on startup'
        );

    }

});