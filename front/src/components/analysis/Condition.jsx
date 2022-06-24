import React, { useState } from 'react';
import PropTypes from 'prop-types';

import { conditionPropTypes } from '../../Analysis';

// import Insights from './Insights';
import Toggle from '../Toggle';


const Item = function({ name, condition }) {
    const value = condition[name] || '';

    if(!value) {
        return (null);
    }

    return (
        <div className="list-group-item">
            <div className="row">
                <div className="col-sm-3">{ name }</div>
                <div className="col-sm-9">{ value }</div>
            </div>
        </div>
    );
};

Item.defaultProps = {};

Item.propTypes = {
    name: PropTypes.string.isRequired,
    condition: conditionPropTypes(),
};


const Details = function({ className, condition }) {

    // condition must have been normalized by a parent
    const { LearnMore, Tips, Recommendation } = condition;

    let len  =0;
    len += LearnMore ? 1 : 0;
    len += Tips ? 1 : 0;
    len += Recommendation ? 1 : 0;

    if (!len) {
        return (null);
    }

    return (
        <div className={ className }>
            {
                (LearnMore || Tips) &&
                <Toggle title={ `More (${len})`} className="list-group mb-2" titleTag="div" titleClassName="text-primary ml-4 pb-2">
                    <Item name="Recommendation" condition={ condition } />
                    <Item name="Learn More" condition={ condition } />
                    <Item name="Tips" condition={ condition } />
                </Toggle>
            }
        </div>
    );
};

Details.defaultProps = {
    className: '',
    condition: null,
};

Details.propTypes = {
    className: PropTypes.string,
    condition: conditionPropTypes(),
};


const Insights = function({ className, condition }) {

    // condition must have been normalized by a parent
    const { Insights } = condition;

    if (!Insights.length) {
        return (null);
    }

    return (
        <div className={ className }>
        {
            <Toggle title={ `Insights (${Insights.length})` } className="mb-2" titleTag="div" titleClassName="text-primary ml-4 pb-2">
                {
                    Insights.map((insight, index) => {
                        return (
                            <div key={ index } className="list-group-item">
                                <strong>{ insight[0] }</strong><br />{ insight[1] }
                            </div>
                        );
                    })
                }
            </Toggle>
        }
        </div>
    );
};

Insights.defaultProps = {
    className: '',
    condition: null,
};

Insights.propTypes = {
    className: PropTypes.string,
    condition: conditionPropTypes(),
};


const Condition = function({ condition, prefix }) {

    // condition must have been normalized by a parent
    const { Description, Condition } = condition;

    return (
        <div className="mb-4">
            <h3>{ Condition }</h3>
            <div className="list-group mb-2">
                <Item
                    name="Category"
                    condition={ condition }
                />

                <Item
                    name="Classification"
                    condition={ condition }
                />

                <div className="list-group-item">{ Description }</div>
            </div>

            <Details condition={ condition } />
            <Insights condition={ condition } />

        </div>
    );
}

Condition.defaultProps = {
    condition: null,
};

Condition.propTypes = {
    condition: conditionPropTypes(),
};

export default Condition;
