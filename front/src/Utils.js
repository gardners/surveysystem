const isScalar = function(v) {
    const type = typeof v;
    return v === null || ['string', 'number', 'boolean'].indexOf(type) > -1;
};


export {
    isScalar
};
