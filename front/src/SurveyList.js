import PropTypes from 'prop-types';

const {
    PUBLIC_URL,
    REACT_APP_SURVEY_LIST,
} = process.env;

const coerce = function(id, data) {

    let name = data.name || '';
    if (data instanceof Error) {
         name = id;
    }

    return {
        id,
        name,
        title: data.title || '',
        description: data.description || '',
        organisation: data.organisation || '',
        email: data.email || '',
        phone: data.phone || '',
        pages: data.pages || [],
        // reponse error
        error: (data instanceof Error) ? data : null,
    };
};

const getById = function(id) {
    const uri = `${PUBLIC_URL}/surveys/${id}/survey.json`;
    return fetch(uri)
    .then(response => response.text())
    .then((body) => {
        try {
            return JSON.parse(body);
        } catch (e) {
            return new Error(`Survey ${id} does not exist or is invalid!`);
        }
    })
    .then(data => coerce(id, data));
};

const getAll = function() {
    const surveyIds = REACT_APP_SURVEY_LIST.split(',').map(name => name.trim());
    return Promise.all(surveyIds.map(id => getById(id)));
};


/**
* Get Proptypes schema
* @returns {PropTypes}
*/
const itemPropTypes = function () {
    return PropTypes.shape({
        id: PropTypes.string,
        name: PropTypes.string,
        title: PropTypes.string,
        description: PropTypes.string,
        organisation: PropTypes.string,
        email: PropTypes.string,
        pages: PropTypes.array,
    });
};

// @see eslint import/no-anonymous-default-export
const ex = { getAll, getById, itemPropTypes };
export default ex;
