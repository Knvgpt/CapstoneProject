import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, Button, Alert } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

const Stack = createStackNavigator();

const HomeScreen = ({ navigation }) => {
  const [name, setName] = useState('');
  const [age, setAge] = useState('');
  const [sex, setSex] = useState('');

  const handleSubmit = () => {
    navigation.navigate('Details', { name, age, sex });
  };

  return (
    <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
      <Text>Name:</Text>
      <TextInput
        style={{ height: 40, borderColor: 'gray', borderWidth: 1, marginBottom: 20, padding: 5 }}
        onChangeText={text => setName(text)}
        value={name}
      />
      <Text>Age (months):</Text>
      <TextInput
        style={{ height: 40, borderColor: 'gray', borderWidth: 1, marginBottom: 20, padding: 5 }}
        onChangeText={text => setAge(text)}
        value={age}
        keyboardType="numeric"
      />
      <Text>Sex:</Text>
      <TextInput
        style={{ height: 40, borderColor: 'gray', borderWidth: 1, marginBottom: 20, padding: 5 }}
        onChangeText={text => setSex(text)}
        value={sex}
      />
      <Button
        title="Submit"
        onPress={handleSubmit}
      />
    </View>
  );
};

const DetailsScreen = ({ route, navigation }) => {
  const { name, age, sex } = route.params;

  return (
    <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
      <Text>Name: {name}</Text>
      <Text>Age (months): {age}</Text>
      <Text>Sex: {sex}</Text>
      <Button
        title="View Results"
        onPress={() => navigation.navigate('Results')}
      />
    </View>
  );
};

const ResultsScreen = () => {
  const [result, setResult] = useState('');

  useEffect(() => {
    fetch('http://your-api-url')
      .then(response => {
        if (!response.ok) {
          throw new Error('Failed to fetch data');
        }
        return response.json();
      })
      .then(data => {
        setResult(data.result);
      })
      .catch(error => {
        console.error('Error:', error);
        Alert.alert('Error', 'Failed to fetch data. Please try again later.');
      });
  }, []);

  return (
    <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
      <Text>Result: {result}</Text>
    </View>
  );
};

const App = () => {
  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName="Home">
        <Stack.Screen name="Home" component={HomeScreen} />
        <Stack.Screen name="Details" component={DetailsScreen} />
        <Stack.Screen name="Results" component={ResultsScreen} />
      </Stack.Navigator>
    </NavigationContainer>
  );
};

export default App;
