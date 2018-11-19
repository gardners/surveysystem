import React, { Component } from 'react';
import PropTypes from 'prop-types';

class Welcome extends Component {
    render() {
        return (
            <div> Survey front page. For tests, try /survey/:id with a valid id to display the survey</div>
        );
    }
}

Welcome.propTypes = {};

export default Welcome;
