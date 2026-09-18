// MinIO service functions
const client = require('../config/storage.js');

const BUCKET_NAME = process.env.MINIO_BUCKET_NAME || 'aigit-chunks';
const EXPIRY_SECONDS = 3600; // URLs remain valid for 1 hour


// Check whether the bucket exists.
// If it does not exist, create it.
const bucket_exists = async () => {
    try {
        const exists = await client.bucketExists(BUCKET_NAME);

        if (!exists) {
            await client.makeBucket(BUCKET_NAME);
            console.log(`Created MinIO bucket: ${BUCKET_NAME}`);
        }
    }
    catch (error) {
        console.error('Error verifying MinIO bucket status:', error);
        throw error;
    }
};


// Check whether a particular CAS object already exists in MinIO.
//
// In our remote CAS, the object key is its SHA-256 hash.
// Therefore, checking for the hash in MinIO tells us whether
// that chunk/manifest has already been uploaded.
const object_exists = async (objectHash) => {
    try {
        await client.statObject(BUCKET_NAME, objectHash);

        // statObject succeeded -> object exists
        return true;
    }
    catch (error) {

        // MinIO returns NotFound / NoSuchKey when the object
        // does not exist. That is not an actual server failure.
        if (
            error.code === 'NotFound' ||
            error.code === 'NoSuchKey' ||
            error.statusCode === 404
        ) {
            return false;
        }

        // Authentication/network/etc. errors should not be
        // mistaken for "object doesn't exist".
        console.error(
            `Failed to check existence of object ${objectHash}:`,
            error
        );

        throw error;
    }
};


// Generate a presigned PUT URL for uploading an object.
const upload_url = async (objectHash) => {
    try {
        return await client.presignedPutObject(
            BUCKET_NAME,
            objectHash,
            EXPIRY_SECONDS
        );
    }
    catch (error) {
        console.error(
            `Failed to generate upload URL for object ${objectHash}:`,
            error
        );

        throw error;
    }
};


// Generate a presigned GET URL for downloading an object.
const download_url = async (objectHash) => {
    try {
        return await client.presignedGetObject(
            BUCKET_NAME,
            objectHash,
            EXPIRY_SECONDS
        );
    }
    catch (error) {
        console.error(
            `Failed to generate download URL for object ${objectHash}:`,
            error
        );

        throw error;
    }
};


module.exports = {
    bucket_exists,
    object_exists,
    upload_url,
    download_url,
};