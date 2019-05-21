import React from 'react';

import Content from './Content';
import Section from './Section';
import Typography from '@material-ui/core/Typography';

const Page404 = function(){
    return (
        <Content title="Not found">
            <Section>
                <Typography variant="h4" component="h2" color="error" >Error 404</Typography> Sorry, this page doesn't exist !
            </Section>
        </Content>
    );
};

Page404.propTypes = {};

export default Page404;
