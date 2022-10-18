import React, { useContext } from 'react';
import PropTypes from 'prop-types';
import { withRouter } from 'react-router-dom';

import { AuthContext } from './AuthProvider';

import ApiAlert from './ApiAlert';
import Preloader from './Preloader';


const Main = function({ style, children }) {
    const auth = useContext(AuthContext);

    return (
        <main className="container" style={ style }>
            <Preloader loading={ auth.loading } message={ auth.loading } />
            { auth.error && <ApiAlert error={ auth.error } /> }
            { !auth.loading && children }
        </main>
    );
}

Main.defaultProps ={
    style: {},
};

Main.propTypes = {
    style: PropTypes.object,
};

export default withRouter(Main);
