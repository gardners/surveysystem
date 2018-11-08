//tuto : https://javascript.info/async-await
// and : https://stackoverflow.com/questions/49500379/typical-file-structure-in-reactjs-application-grouping-api-calls-in-api-js

export async function resolve(promise) {
    const resolved = {
        response: null,
        error: null
    };

    try {
        resolved.response = await promise;
    } catch(e) {
        resolved.error = e;
    }
    return resolved;
}